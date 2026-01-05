#include "allocation.h"

struct allocated_memory
{
    void *start_addr;
    void *finish_addr;
};


struct allocated_memory *memory_allocations;
int memory_allocations_index;
int memory_used = 0;
uint64_t memory_available = 0;
char *memory = 0;

static EFI_MEMORY_DESCRIPTOR *find_memory_start(struct memory_map *map)
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

void memory_init(struct memory_map *map)
{
    EFI_MEMORY_DESCRIPTOR *memory_desc = find_memory_start(map);
    memory_used = 0;
    memory_available = memory_desc->NumberOfPages * 4096;
    memory = (char *)memory_desc->PhysicalStart;

    memory_allocations = (struct allocated_memory *)memory;
    memory += 4096;
    memory_available = (memory_desc->NumberOfPages - 1) * 4096;
    memory_allocations_index = 0;
}

void *malloc(uint64_t size)
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
    return 0;
}

uint64_t get_available_memory()
{
    return memory_available;
}