#include "../gnu-efi/inc/efi.h"
#include "font.h"
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#define ISR_ERR_STUB(n) __attribute__((interrupt))\
                                                    void isr_stub_##n(void *stack_frame, unsigned long code)\
                                                    {\
                                                        exception();\
                                                    }
#define ISR_NO_ERR_STUB(n) __attribute__((interrupt)) \
                                                      void isr_stub_##n(void *stack_frame)\
                                                      {\
                                                          exception();\
                                                      }
#define GET_ISR(n) isr_stub_##n()
int memory_used = 0;
UINT64 memory_available = 0;
char *memory = NULL;
int letters_available = 0;
int letters_used = 0;
int rows_available = 0;
int rows_used = 0;
UINT32 width;
UINT32 *framebuffer;

struct memory_map
{
    UINTN size;
    EFI_MEMORY_DESCRIPTOR *mem_map;
    UINTN key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

struct allocated_memory
{
    void *start_addr;
    void *finish_addr;
};

struct allocated_memory *memory_allocations;
int memory_allocations_index;

typedef struct 
{
    struct memory_map map;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE graphics_mode;
    EFI_RUNTIME_SERVICES *runtime_services;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *config_table; 
} kernel_params;

typedef struct {
    uint16_t    isr_low;
    uint16_t    kernel_cs;
    uint8_t	    ist;
    uint8_t     attributes;
    uint16_t    isr_mid;
    uint32_t    isr_high;
    uint32_t    reserved;
} __attribute__((packed)) idt_entry;

typedef struct {
    uint16_t	limit;
    uint64_t	base;
} __attribute__((packed)) idtr;

static idt_entry idt[256];
static idtr idtr_;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flag_limit;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdtr;

static gdt_entry gdt[3];
static gdtr gdtr_;
char rtl8139_interrupt;
char rtl8139_nic_number;
uint32_t rtl8139_io_addr;

EFI_MEMORY_DESCRIPTOR *find_memory_start(struct memory_map *map);
static inline UINT32 inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile ("inl %1, %0": "=a"(ret) : "Nd"(port));
    return ret;
}


static inline UINT16 inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile ("inw %1, %0": "=a"(ret) : "Nd"(port));
    return ret;
}

static inline UINT8 inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0": "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}


static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void render_font(char c, UINT32 *framebuffer, UINT32 width, int fb_y_start, int fb_x_start);
void print(char *c, UINT32 *framebuffer, UINT32 width);
void panic(char *message, UINT32 *framebuffer);
void exception();
void printn(char *c, UINT32 *framebuffer, size_t n);
void putchar(char c, UINT32 *framebuffer);
void putnumber(INT64 num, UINT32 *framebuffer);
void puthex(UINT64 num, UINT32 *framebuffer);
void printf(UINT32 *framebuffer, const char *format, ...);
void *malloc(UINT64 size);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dst, void *src, size_t n);
void write_to_address(UINT64 addr, void *data, size_t size);
uint16_t read_from_pci(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
uint32_t read_from_pci_whole(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void write_to_pci(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t to_write);
void idt_init();
void gdt_init();
void pic_init();
static inline unsigned char are_interrupts_enabled()
{
    unsigned long flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(flags) );
    return flags & (1 << 9);
}
static inline void io_wait(void)
{
    outb(0x80, 0);
}

void __attribute__((ms_abi)) kernel_main(kernel_params params)
{
    framebuffer = (UINT32 *)params.graphics_mode.FrameBufferBase;
    UINT32 height = params.graphics_mode.Info->VerticalResolution;
    width = params.graphics_mode.Info->PixelsPerScanLine;
    EFI_MEMORY_DESCRIPTOR *memory_desc = find_memory_start(&params.map);
    char *rx_buffer = malloc(8192 + 16 + 1500); //WRAP is 1
    memory_used = 0;
    memory_available = memory_desc->NumberOfPages * 4096;
    memory = (char *)memory_desc->PhysicalStart;

    memory_allocations = (struct allocated_memory *)memory;
    memory += 4096;
    memory_available = (memory_desc->NumberOfPages - 1) * 4096;
    memory_allocations_index = 0;
    letters_available = (width-30)/9;
    rows_available = height/15;
    letters_used = 0;
    rows_used = 0;

    for (INT32 y = 0; y < height; y++)
    {
        for(INT32 x = 0; x < width; x++)
        {
            framebuffer[y * width + x] = 0xFF000000; //Red AARRGGBB
        }
    }
    gdt_init();
    pic_init();
    idt_init();
    printf(framebuffer, "Are intrrupts enabled? %i\n", are_interrupts_enabled());
    printf(framebuffer, "Characters %ix%i\n", letters_available, rows_available);
    printf(framebuffer, "Memory available %iMB\n", memory_available/(1024 * 1024));
    printf(framebuffer, "Number of table services %i\n", params.NumberOfTableEntries);
    char *acpi_table = NULL;
    for (int i = 0; i < params.NumberOfTableEntries; i++)
    {
        EFI_GUID guid = params.config_table[i].VendorGuid;
        EFI_GUID acpi_guid = (EFI_GUID)ACPI_20_TABLE_GUID ;
        if (memcmp(&guid, &acpi_guid, sizeof(EFI_GUID)) == 0)
        {
            acpi_table = params.config_table[i].VendorTable;
            printf(framebuffer, "Got the ACPI table\n");
        }
    }
    if (!acpi_table)
        panic("Acpi table not found", framebuffer);
    printf(framebuffer, "Acpi table signature: ");
    printn(acpi_table, framebuffer, 8);
    printf(framebuffer, "\n");
    printf(framebuffer, "Acpi table OEM: ");
    printn(&acpi_table[9], framebuffer, 6);
    printf(framebuffer, "\n");
    /*uintptr_t acpi_table_addr = 0;
    memcpy(&acpi_table_addr, &acpi_table[16], sizeof(INT32));
    char *acpi_ = (void *)acpi_table_addr;
    INT32 lenght = 0;
    memcpy(&lenght, &acpi_table[20], sizeof(INT32));*/
    printf(framebuffer, "address of acpi table is 0x%x lenght of acpi table is %i\n", *(UINT32 *)&acpi_table[16], *(UINT32 *)&acpi_table[20]);
    char *xsdt = (void *)*(UINT64 *)&acpi_table[24];
    int xsdt_lenght = *(int *)&xsdt[4];
    int address_entries = (xsdt_lenght - 36)/8;
    printf(framebuffer, "XSDT number of entries: %i\n", address_entries);
    for (int i = 0; i < address_entries; i++)
    {
        printf(framebuffer, "entry %i signature: ", i);
        char *address = (void *)*(UINT64 *)&xsdt[36 + (8 * i)];
        printn(address, framebuffer, 4);
        printf(framebuffer, "\n");
    }
    UINT64 data_addr = 0xCFC;
    char found = 0;
    int dev_found = 0;
    for (int device = 0; device < 32; device++)
    {
        uint16_t vendor_id = read_from_pci(0, device, 0, 0);
        uint16_t device_id = read_from_pci(0, device, 0, 2);
        printf(framebuffer,"pci dev %i vendor id %x device id %x\n", device, vendor_id, device_id);
        if (vendor_id == 0x10EC && device_id == 0x8139)
        {
            printf(framebuffer, "Found RTL8139\n");
            found = 1;
            dev_found = device;
            break;
        }
        
    }
    if (found)
    {
        uint32_t header_type_ = read_from_pci_whole(0, dev_found, 0, 0xC);
        uint8_t header_type = ((char *)&header_type_)[3];
        rtl8139_interrupt = 0;
        rtl8139_nic_number = 0;
        rtl8139_io_addr = 0;
        uint16_t command = read_from_pci(0, dev_found, 0, 0x4);
        //TODO Initialize RTL8139 properly
        printf(framebuffer, "header type %i\n", header_type);
        if (header_type != 0)
            panic("Device is not as expected", framebuffer);
        uint32_t io_addr = 0x0;
        uint32_t last_offset = read_from_pci_whole(0, dev_found, 0, 0x3C);
        uint8_t irq_channel = 0;
        memcpy(&irq_channel, &last_offset, sizeof(uint8_t));
        rtl8139_nic_number = irq_channel - 9;
        printf(framebuffer, "irq channel of device is %i %i\n", irq_channel, rtl8139_nic_number);
        for (int i = 0x10; i < 0x24; i += 4)
        {
            uint32_t bar = read_from_pci_whole(0, dev_found, 0, i);
            if (i == 0x10)
                printf(framebuffer, "Bar %i: 0x%x\n", 0, bar);
            else
                printf(framebuffer, "Bar %i: 0x%x\n", (i - 0x10)/4, bar);
            if (bar & (1 << 0) && bar != 0x0)
            {
                printf(framebuffer, "I/O port bar found %x\n", bar);
                io_addr = bar & 0xFFFFFFFC;
                break;
            }
        }
        if (!io_addr)
            panic("io port address for RTL8139 not found", framebuffer);
        rtl8139_io_addr = io_addr;
        printf(framebuffer, "i/o BAR address %x\n", io_addr);
        uint32_t mac1 = (uint32_t)inl(io_addr);
        uint16_t mac2 = (uint16_t)inw(io_addr + 4);
        printf(framebuffer, "mac address %x%x\n", mac2, mac1);
        outb(io_addr + 0x52, 0x0);
        printf(framebuffer, "Turned on RTL8139\n");
        outb(io_addr + 0x37, 0x10);
        while((inb(io_addr + 0x37) & 0x10) != 0);
        printf(framebuffer, "Resetted RTL8139\n");
        outl(io_addr + 0x30, (uintptr_t)rx_buffer);
        printf(framebuffer, "receive buffer initialized\n");
        outw(io_addr + 0x3C, 0x0005);
        printf(framebuffer, "Turned on interrupts for RTL8139\n");
        outl(io_addr + 0x44, 0xf | (1 << 7));
        outb(io_addr + 0x37, 0x0C);
    }
    while(1);
}

EFI_MEMORY_DESCRIPTOR *find_memory_start(struct memory_map *map)
{
    EFI_MEMORY_DESCRIPTOR *largest = NULL;
    for (int i = 0; i < map->size / map->descriptor_size; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)map->mem_map + (i * map->descriptor_size));
        if (desc->Type == EfiConventionalMemory)
        {
            if (!largest)
                largest = desc;
            else if (largest->NumberOfPages < desc->NumberOfPages)
                largest = desc;
        }
    }
    return largest;
}

void render_font(char c, UINT32 *framebuffer, UINT32 width, int fb_y_start, int fb_x_start)
{
    int x,y;
    int set;
    int fb_y = fb_y_start;
    int fb_x = fb_x_start;
    for (x=0; x < 8; x++) 
    {
        for (y=0; y < 8; y++) 
        {
            set = font8x8_basic[c][x] & 1 << y;
            if (set)
                framebuffer[(fb_y * width) + fb_x] = 0xFFFFFFFF;
            fb_x++;
        }
        fb_y++;
        fb_x = fb_x_start;
    }
}



void print(char *c, UINT32 *framebuffer, UINT32 width)
{
    while (*c)
    {
        if (letters_used >= letters_available || *c == '\n')
        {
            rows_used++;
            letters_used = 0;
            if (*c == '\n')
            {
                c++;
                continue;
            }
        }
        render_font(*c, framebuffer, width, 30 + (rows_used * 15), 30 + (letters_used * 9));
        letters_used++;
        c++;
    }
}

void panic(char *message, UINT32 *framebuffer)
{
    print("PANIC! ", framebuffer, width);
    print(message, framebuffer, width);
    asm( "hlt" );
}

void putchar(char c, UINT32 *framebuffer)
{
    if (letters_used >= letters_available || c == '\n')
    {
        rows_used++;
        letters_used = 0;
        if (c == '\n')
            return;
    }
    render_font(c, framebuffer, width, 30 + (rows_used * 15), 30 + (letters_used * 9));
    letters_used++;
    c++;
}

void printn(char *c, UINT32 *framebuffer, size_t n)
{
    int i = 0;
    while (i < n)
    {
        if (letters_used >= letters_available || *c == '\n')
        {
            rows_used++;
            letters_used = 0;
            if (*c == '\n')
            {
                c++;
                continue;
            }
        }
        render_font(*c, framebuffer, width, 30 + (rows_used * 15), 30 + (letters_used * 9));
        letters_used++;
        c++;
        i++;
    }
}

void putnumber(INT64 num, UINT32 *framebuffer)
{
    if (num == -2147483648)
		return (print("-2147483648", framebuffer, width));
    if (num < 0)
    {
        putchar('-', framebuffer);
        num = num * -1;
    }
    if (num >= 10)
    {
        putnumber(num/10, framebuffer);
        num = num % 10;
    }
    if (num < 10)
        putchar(num + '0', framebuffer);
}

void puthex(UINT64 num, UINT32 *framebuffer)
{
    if (num >= 16)
    {
        puthex(num/16, framebuffer);
        num = num % 16;
    }
    if (num > 9)
    {
        putchar((num - 10) + 'A', framebuffer);
    }
    if (num <= 9)
        putchar(num + '0', framebuffer);
}

void printf(UINT32 *framebuffer, const char *format, ...)
{
    va_list pointer;

    va_start(pointer, format);
    while (*format)
    {
        if (!format)
            break;
        if (*format == '%')
        {
            format++;
            if (!*format)
                break;
            switch (*format)
            {
                case 'i':
                    putnumber((unsigned int)va_arg(pointer, unsigned int), framebuffer);
                    break;
                case 's':
                    print((char *)va_arg(pointer, char *), framebuffer, width);
                    break;
                case 'x':
                    puthex((unsigned int)va_arg(pointer, unsigned int), framebuffer);
                    break;
            }
        }
        else 
            putchar(*format, framebuffer);
        format++;
    }
}

void *malloc(UINT64 size)
{
    int allocations_index = memory_allocations_index;
    if (memory_available > size && memory_allocations_index <= 256)
    {
        memory_allocations[memory_allocations_index].start_addr = memory;
        memory += size;
        memory_allocations[memory_allocations_index].finish_addr = memory;
        memory_available -= size;
        memory_allocations_index++;
        return memory_allocations[allocations_index].start_addr;
    }
    return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    char *s1_ = (char *)s1;
    char *s2_ = (char *)s2;

    for (int i = 0; i < n; i++)
    {
        if ((unsigned char)s1_[i] != (unsigned char)s2_[i])
            return (unsigned char)s1_[i] - (unsigned char)s2_[i];
    }
    return 0;
}

void *memcpy(void *dst, void *src, size_t n)
{
    int i = 0;
    char *ret = dst;
    char *d = dst;
    char *s = src;
    while(i < n)
    {
        *d = *s;
        d++;
        s++;
        i++;
    }
    return ret;
}

void write_to_address(UINT64 addr, void *data, size_t size)
{
    char *a = (char *)*(UINT64 *)&addr;
    memcpy(a, data, size);
}


uint16_t read_from_pci(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
        uint32_t address = (uint32_t)((0 << 16) | (device << 11) |
              (0 << 8) | (0 & 0xFC) | ((uint32_t)0x80000000));
        outl(0xCF8, address);
        //write_to_address(0xCF8, &address, 4);
        uint16_t ret = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
        return ret;
}


uint32_t read_from_pci_whole(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
        uint32_t address = (uint32_t)((bus << 16) | (device << 11) |
              (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        outl(0xCF8, address);
        //write_to_address(0xCF8, &address, 4);
        uint32_t ret = (uint32_t)inl(0xCFC);
        return ret;
}


void write_to_pci(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t to_write)
{
        uint32_t address = (uint32_t)((0 << 16) | (device << 11) |
              (0 << 8) | (0 & 0xFC) | ((uint32_t)0x80000000));
        outl(0xCF8, address);
        outw(0xCFC + (offset & 2), to_write);
}

ISR_NO_ERR_STUB(0);
ISR_NO_ERR_STUB(1);
ISR_NO_ERR_STUB(2);
ISR_NO_ERR_STUB(3);
ISR_NO_ERR_STUB(4);
ISR_NO_ERR_STUB(5);
ISR_NO_ERR_STUB(6);
ISR_NO_ERR_STUB(7);
ISR_ERR_STUB(8);
ISR_NO_ERR_STUB(9);
ISR_ERR_STUB(10);
ISR_ERR_STUB(11);
ISR_ERR_STUB(12);
ISR_ERR_STUB(13);
ISR_ERR_STUB(14);
ISR_NO_ERR_STUB(15);
ISR_NO_ERR_STUB(16);
ISR_ERR_STUB(17);
ISR_NO_ERR_STUB(18);
ISR_NO_ERR_STUB(19);
ISR_NO_ERR_STUB(20);
ISR_NO_ERR_STUB(21);
ISR_NO_ERR_STUB(22);
ISR_NO_ERR_STUB(23);
ISR_NO_ERR_STUB(24);
ISR_NO_ERR_STUB(25);
ISR_NO_ERR_STUB(26);
ISR_NO_ERR_STUB(27);
ISR_NO_ERR_STUB(28);
ISR_NO_ERR_STUB(29);
ISR_ERR_STUB(30);
ISR_NO_ERR_STUB(31);

__attribute__((interrupt))
void timer(void *stack_frame)
{
    printf(framebuffer, "Timer!");
    outb(0x20, 0x20); 
}

__attribute__((interrupt))
void key_pressed(void *stack_frame)
{
    outb(0x20, 0x20);
}


__attribute__((interrupt))
void nic_irq_0(void *ret)
{
    if (rtl8139_nic_number == 0)
    {
        uint16_t status = inw(rtl8139_io_addr + 0x3E);
        outw(rtl8139_io_addr + 0x3E, 0x05);
        if (status & 0x01)
        {
            printf(framebuffer, "Receiving a packet\n");
        }
        if (status & 0x04)
        {
            printf(framebuffer, "Sent a packet\n");
        }
        
        rtl8139_interrupt = 1;
        printf(framebuffer, "Got RTL8139 interrupt\n");
    }
}


__attribute__((interrupt))
void nic_irq_1(void *ret)
{  
    if (rtl8139_nic_number == 1)
    {
        uint16_t status = inw(rtl8139_io_addr + 0x3E);
        outw(rtl8139_io_addr + 0x3E, 0x05);
        if (status & 0x01)
        {
            printf(framebuffer, "Receiving a packet\n");
        }
        if (status & 0x04)
        {
            printf(framebuffer, "Sent a packet\n");
        }
        
        rtl8139_interrupt = 1;
        printf(framebuffer, "Got RTL8139 interrupt\n");
    }
}


__attribute__((interrupt))
void nic_irq_2(void *ret)
{
    if (rtl8139_nic_number == 2)
    {
        uint16_t status = inw(rtl8139_io_addr + 0x3E);
        outw(rtl8139_io_addr + 0x3E, 0x05);
        if (status & 0x01)
        {
            printf(framebuffer, "Receiving a packet\n");
        }
        if (status & 0x04)
        {
            printf(framebuffer, "Sent a packet\n");
        }
        
        rtl8139_interrupt = 1;
        printf(framebuffer, "Got RTL8139 interrupt\n");
    }
}


static void *isr_stub_table[47] =
{
    isr_stub_0, isr_stub_1, isr_stub_2, isr_stub_3,
    isr_stub_4, isr_stub_5, isr_stub_6, isr_stub_7,
    isr_stub_8, isr_stub_9, isr_stub_10, isr_stub_11,
    isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15,
    isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19,
    isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23,
    isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27,
    isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31, timer, key_pressed
};


void exception()
{
    panic("Exception!", framebuffer);
    //__asm__ volatile ("cli; hlt");
}

void idt_init()
{
    idtr_.base = (uintptr_t)&idt[0];
    idtr_.limit = (uint16_t)sizeof(idt_entry) * 44 - 1;
    isr_stub_table[0] = (void *)isr_stub_0;
    isr_stub_table[1] = (void *)isr_stub_1;
    isr_stub_table[2] = (void *)isr_stub_2;
    isr_stub_table[3] = (void *)isr_stub_3;
    isr_stub_table[4] = (void *)isr_stub_4;
    isr_stub_table[5] = (void *)isr_stub_5;
    isr_stub_table[6] = (void *)isr_stub_6;
    isr_stub_table[7] = (void *)isr_stub_7;
    isr_stub_table[8] = (void *)isr_stub_8;
    isr_stub_table[9] = (void *)isr_stub_9;
    isr_stub_table[10] = (void *)isr_stub_10;
    isr_stub_table[11] = (void *)isr_stub_11;
    isr_stub_table[12] = (void *)isr_stub_12;
    isr_stub_table[13] = (void *)isr_stub_13;
    isr_stub_table[14] = (void *)isr_stub_14;
    isr_stub_table[15] = (void *)isr_stub_15;
    isr_stub_table[16] = (void *)isr_stub_16;
    isr_stub_table[17] = (void *)isr_stub_17;
    isr_stub_table[18] = (void *)isr_stub_18;
    isr_stub_table[19] = (void *)isr_stub_19;
    isr_stub_table[20] = (void *)isr_stub_20;
    isr_stub_table[21] = (void *)isr_stub_21;
    isr_stub_table[22] = (void *)isr_stub_22;
    isr_stub_table[23] = (void *)isr_stub_23;
    isr_stub_table[24] = (void *)isr_stub_24;
    isr_stub_table[25] = (void *)isr_stub_25;
    isr_stub_table[26] = (void *)isr_stub_26;
    isr_stub_table[27] = (void *)isr_stub_27;
    isr_stub_table[28] = (void *)isr_stub_28;
    isr_stub_table[29] = (void *)isr_stub_29;
    isr_stub_table[30] = (void *)isr_stub_30;
    isr_stub_table[31] = (void *)isr_stub_31;
    isr_stub_table[32] = (void *)timer;
    isr_stub_table[33] = (void *)key_pressed;
    isr_stub_table[41] = (void *)nic_irq_0;
    isr_stub_table[42] = (void *)nic_irq_1;
    isr_stub_table[43] = (void *)nic_irq_2;
    
    for (int i = 0; i < 44; i++)
    {
        idt_entry *descriptor = &idt[i];

        descriptor->isr_low = (uint64_t)isr_stub_table[i] & 0xFFFF;
        descriptor->kernel_cs = 0x8;
        descriptor->ist = 0;
        descriptor->attributes = 0x8E;
        descriptor->isr_mid = ((uint64_t)isr_stub_table[i] >> 16) & 0xFFFF;
        descriptor->isr_high = ((uint64_t)isr_stub_table[i] >> 32) & 0xFFFFFFFF;
        descriptor->reserved = 0;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr_));
    __asm__ volatile ("sti");
}

void gdt_init()
{
    __asm__ volatile("cli");
    gdt[0] = (gdt_entry){0, 0, 0, 0, 0, 0};

    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0;
    gdt[1].base_middle = 0;
    gdt[1].access = 0x9A;
    gdt[1].flag_limit = 0xAF;
    gdt[1].base_high = 0;

    gdt[2].limit_low = 0xFFFF;
    gdt[2].base_low = 0;
    gdt[2].base_middle = 0;
    gdt[2].access = 0x92;
    gdt[2].flag_limit = 0xCF;
    gdt[2].base_high = 0;

    gdtr_.limit = sizeof(gdt) - 1;
    gdtr_.base = (uint64_t)&gdt;
    
    __asm__ volatile("lgdt %0" : : "m"(gdtr_));
    __asm__ volatile("pushq $0x08\n leaq 1f(%%rip), %%rax \n pushq %%rax\n lretq\n 1:\n mov $0x10, %%ax\n mov %%ax, %%ds\n mov %%ax, %%es\n mov %%ax, %%fs\n mov %%ax, %%gs\n mov %%ax, %%ss\n" :::"rax", "ax");
}

void pic_init()
{
    outb(0x20, 0x10 | 0x01);
    io_wait();
    outb(0xA0, 0x10 | 0x01);
    io_wait();
    outb(0x20 + 1, 32);
    io_wait();
    outb(0xA0 + 1, 40);
    io_wait();
    outb(0x20 + 1, 1 << 2);
    io_wait();
    outb(0xA0 + 1, 2);
    io_wait();
    outb(0x20 + 1, 0x01);
    io_wait();
    outb(0xA0 + 1, 0x01);
    io_wait();

    outb(0x20 + 1, 0);
    outb(0xA0 + 1, 0);
}
