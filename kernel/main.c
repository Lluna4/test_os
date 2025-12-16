#include "/usr/include/efi/efi.h"
#include "font.h"

int memory_used = 0;
int memory_available = 0;
char *memory = NULL;
int letters_available = 0;
int letters_used = 0;
int rows_available = 0;
int rows_used = 0;

struct memory_map
{
    UINTN size;
    EFI_MEMORY_DESCRIPTOR *mem_map;
    UINTN key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

typedef struct 
{
    struct memory_map map;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE graphics_mode;
} kernel_params;

EFI_MEMORY_DESCRIPTOR *find_memory_start(struct memory_map *map);
void render_font(char c, UINT32 *framebuffer, UINT32 width, int fb_y_start, int fb_x_start);
void print(char *c, UINT32 *framebuffer, UINT32 width);

void __attribute__((ms_abi)) kernel_main(kernel_params params)
{
    UINT32 *framebuffer = (UINT32 *)params.graphics_mode.FrameBufferBase;
    UINT32 height = params.graphics_mode.Info->VerticalResolution;
    UINT32 width = params.graphics_mode.Info->PixelsPerScanLine;
    EFI_MEMORY_DESCRIPTOR *memory_desc = find_memory_start(&params.map);
    memory_used = 0;
    memory_available = memory_desc->NumberOfPages * 4096;
    memory = (char *)memory_desc->PhysicalStart;

    letters_available = (width-30)/9;
    rows_available = height/10;
    letters_used = 0;
    rows_used = 0;

    for (INT32 y = 0; y < height; y++)
    {
        for(INT32 x = 0; x < width; x++)
        {
            framebuffer[y * width + x] = 0xFF000000; //Red AARRGGBB
        }
    }

    print("Hi!1821692hauishbaiskabsi18972192aosabsia8siuasiagsi1297819281hsaosqasjoasaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", framebuffer, width);
    while(1);
}

EFI_MEMORY_DESCRIPTOR *find_memory_start(struct memory_map *map)
{
    for (int i = 0; i < map->size / map->descriptor_size; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)map->mem_map + (i * map->descriptor_size));
        if (desc->Type == EfiConventionalMemory)
            return desc;
    }
    return NULL;
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
        if (letters_used >= letters_available)
        {
            rows_used++;
            letters_used = 0;
        }
        render_font(*c, framebuffer, width, 30 + (rows_used * 10), 30 + (letters_used * 9));
        letters_used++;
        c++;
    }
}