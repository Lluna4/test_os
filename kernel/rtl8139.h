#ifndef RTL8139_H
#define RTL8139_H

#include "utils.h"
#include "allocation.h"
#include <stdint.h>

typedef struct
{
    uint8_t mac_addr[6];
    uint32_t rtl8139_io_addr;
    char rtl8139_interrupt;
    int err;
} rtl8139_dev;

enum RTL8139_INIT_ERROR
{
    RTL8139_DEVICE_HEADER_BAD = -1,
    RTL8139_IO_NOT_FOUND = -2
};

rtl8139_dev rtl8139_init(UINT32 *framebuffer, void (*printf)(UINT32 *framebuffer, const char *format, ...));

int rtl8139_send(void *packet, uint32_t packet_size, uint32_t size);
int rtl8139_recv(void *buf, uint32_t size, char *pkt_re);
uint8_t check_sent();
#endif