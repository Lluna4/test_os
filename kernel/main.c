#include "../gnu-efi-code/inc/efi.h"
#include "font.h"
#include <stdarg.h>

int memory_used = 0;
UINT64 memory_available = 0;
char *memory = NULL;
int letters_available = 0;
int letters_used = 0;
int rows_available = 0;
int rows_used = 0;
UINT32 width;

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

EFI_MEMORY_DESCRIPTOR *find_memory_start(struct memory_map *map);
void render_font(char c, UINT32 *framebuffer, UINT32 width, int fb_y_start, int fb_x_start);
void print(char *c, UINT32 *framebuffer, UINT32 width);
void panic(char *message, UINT32 *framebuffer);
void printn(char *c, UINT32 *framebuffer, size_t n);
void putchar(char c, UINT32 *framebuffer);
void putnumber(INT64 num, UINT32 *framebuffer);
void puthex(UINT64 num, UINT32 *framebuffer);
void printf(UINT32 *framebuffer, const char *format, ...);
void *malloc(UINT64 size);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dst, void *src, size_t n);


void __attribute__((ms_abi)) kernel_main(kernel_params params)
{
    UINT32 *framebuffer = (UINT32 *)params.graphics_mode.FrameBufferBase;
    UINT32 height = params.graphics_mode.Info->VerticalResolution;
    width = params.graphics_mode.Info->PixelsPerScanLine;
    EFI_MEMORY_DESCRIPTOR *memory_desc = find_memory_start(&params.map);
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
        putchar(num + 'A', framebuffer);
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
    if (memory_available > size && memory_allocations_index <= 256)
    {
        memory_allocations[memory_allocations_index].start_addr = memory;
        memory += size;
        memory_allocations[memory_allocations_index].finish_addr = memory;
        memory_available -= size;

        return memory_allocations[memory_allocations_index].start_addr;
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
    while(i < n)
    {
        *(char *)dst = *(char *)src;
        (char *)dst++;
        (char *)src++;
        i++;
    }
    return ret;
}