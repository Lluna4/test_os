#ifndef ALLOCATION_H
#define ALLOCATION_H

#include "../gnu-efi/inc/efi.h"

struct memory_map
{
    UINTN size;
    EFI_MEMORY_DESCRIPTOR *mem_map;
    UINTN key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

void memory_init(struct memory_map *map);
void *malloc(uint64_t size);
uint64_t get_available_memory();

#endif