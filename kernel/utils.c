#include "utils.h"

int memcmp(const void *s1, const void *s2, unsigned long long n)
{
    for (int i = 0; i < n; i++)
    {
        if (((unsigned char *)s1)[i] != ((unsigned char *)s2)[i])
            return ((unsigned char *)s1)[i] - ((unsigned char *)s2)[i];
    }
    return 0;
}

void *memcpy(void *dst, void *src, unsigned long long n)
{
    for (int i = 0; i < n; i++)
    {
        ((char *)dst)[i] = ((char *)src)[i];
    }

    return dst;
}

void *memset(void *dst, char n, size_t size)
{
    for (int i = 0; i < size; i++)
        ((char *)dst)[i] = n;
    return dst;
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
