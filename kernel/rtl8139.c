#include "rtl8139.h"

char rtl8139_interrupt;
char rtl8139_nic_number;
uint32_t rtl8139_io_addr;
char *rx_buffer;

static UINT32 *fb;
void (*printff)(UINT32 *framebuffer, const char *format, ...);


int rtl8139_init(UINT32 *framebuffer, void (*printf)(UINT32 *framebuffer, const char *format, ...))
{
    UINT64 data_addr = 0xCFC;
    char found = 0;
    int dev_found = 0;
    char *rx_buffer = malloc(8192 + 16 + 1500); //WRAP is 1
    fb = framebuffer;
    printff = printf;
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
            return RTL8139_DEVICE_HEADER_BAD;
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
            return RTL8139_IO_NOT_FOUND;
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
    return 0;
}

__attribute__((interrupt))
void rtl8139_interrupt_fn(void *stack_frame)
{
    uint16_t status = inw(rtl8139_io_addr + 0x3E);
    outw(rtl8139_io_addr + 0x3E, 0x05);
    if (status & 0x01)
    {
        printff(fb, "Receiving a packet\n");
    }
    if (status & 0x04)
    {
        printff(fb, "Sent a packet\n");
    }
    
    rtl8139_interrupt = 1;
    printff(fb, "Got RTL8139 interrupt\n");
}