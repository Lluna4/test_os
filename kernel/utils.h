#ifndef UTILS_H
#define UTILS_H
#include "../gnu-efi/inc/efi.h"

int memcmp(const void *s1, const void *s2, unsigned long long n);
void *memcpy(void *dst, void *src, unsigned long long n);
void *memset(void *dst, char n, size_t size);
uint16_t read_from_pci(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
uint32_t read_from_pci_whole(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void write_to_pci(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t to_write);

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

static inline void io_wait(void)
{
    outb(0x80, 0);
}


#endif
