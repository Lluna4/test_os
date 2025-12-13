#include "/usr/include/efi/efi.h"

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

void __attribute__((ms_abi)) kernel_main(kernel_params params)
{
    UINT32 *framebuffer = (UINT32 *)params.graphics_mode.FrameBufferBase;
    UINT32 height = params.graphics_mode.Info->VerticalResolution;
    UINT32 width = params.graphics_mode.Info->PixelsPerScanLine;
    for (INT32 y = 0; y < height; y++)
    {
        for(INT32 x = 0; x < width; x++)
        {
            framebuffer[y * width + x] = 0xFFFF0000; //Red AARRGGBB
        }
    }
    while(1);
}