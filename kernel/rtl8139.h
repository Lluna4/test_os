#ifndef RTL8139_H
#define RTL8139_H

#include "utils.h"
#include "allocation.h"

enum RTL8139_INIT_ERROR
{
    RTL8139_DEVICE_HEADER_BAD = -1,
    RTL8139_IO_NOT_FOUND = -2
};

int rtl8139_init(UINT32 *framebuffer, void (*printf)(UINT32 *framebuffer, const char *format, ...));

#endif