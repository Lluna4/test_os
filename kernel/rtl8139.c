#include "rtl8139.h"
#include <stdint.h>

char rtl8139_interrupt;
char rtl8139_nic_number;
uint32_t rtl8139_io_addr;
uint8_t packet_sent;
char rx_buffer[8192 + 16 + 1500];
char tx_buffer[2048];
uint8_t mac_addr[6];

static UINT32 *fb;
void (*printff)(UINT32 *framebuffer, const char *format, ...);


rtl8139_dev rtl8139_init(UINT32 *framebuffer, void (*printf)(UINT32 *framebuffer, const char *format, ...))
{
    rtl8139_dev ret = {0};
    UINT64 data_addr = 0xCFC;
    char found = 0;
    int dev_found = 0;
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
        uint16_t command = read_from_pci_whole(0, dev_found, 0, 0x4);
        //TODO Initialize RTL8139 properly
        command |= (1 << 2);
        write_to_pci(0, dev_found, 0, 0x04, command);
        printf(framebuffer, "header type %i\n", header_type);
        if (header_type != 0)
        {
            ret.err = RTL8139_DEVICE_HEADER_BAD;
            return ret;
        }
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
        {
            ret.err = RTL8139_IO_NOT_FOUND;
            return ret;
        }
        rtl8139_io_addr = io_addr;
        printf(framebuffer, "i/o BAR address %x\n", io_addr);
        uint32_t mac1 = (uint32_t)inl(io_addr);
        uint16_t mac2 = (uint16_t)inw(io_addr + 4);

        mac_addr[0] = mac1 >> 0;
        mac_addr[1] = mac1 >> 8;
        mac_addr[2] = mac1 >> 16;
        mac_addr[3] = mac1 >> 24;
        mac_addr[4] = mac2 >> 0;
        mac_addr[5] = mac2 >> 8;
        memcpy(ret.mac_addr, mac_addr, 6);
        printf(framebuffer, "mac address %x:%x:%x:%x:%x:%x\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        outb(io_addr + 0x52, 0x0);
        printf(framebuffer, "Turned on RTL8139\n");
        outb(io_addr + 0x37, 0x10);
        while((inb(io_addr + 0x37) & 0x10) != 0);
        printf(framebuffer, "Resetted RTL8139\n");
        outl(io_addr + 0x30, (uintptr_t)rx_buffer);
        printf(framebuffer, "receive buffer initialized\n");
        outw(io_addr + 0x3C, 0x0005);
        printf(framebuffer, "Turned on interrupts for RTL8139\n");
        outl(io_addr + 0x44, 0x0F);
        outb(io_addr + 0x37, 0x0C);
        packet_sent = 0;
        memset(tx_buffer, 0, 2048);
        ret.rtl8139_interrupt = rtl8139_interrupt;
        ret.rtl8139_io_addr = rtl8139_io_addr;
        ret.err = 0;
    }
    
    return ret;
}

int rtl8139_send(void *packet, uint32_t packet_size, uint32_t size)
{
    memcpy(tx_buffer, packet, packet_size);

    outl(rtl8139_io_addr + 0x20, (uintptr_t)tx_buffer);
    outl(rtl8139_io_addr + 0x10, size);

    return size;
}

int rtl8139_recv(void *buf, uint32_t size, char *pkt_re)
{
    if (!pkt_re)
        return 0;
    memcpy(buf, rx_buffer, size);

    return size;
}

uint8_t check_sent()
{
    return packet_sent;
}