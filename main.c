#include "gnu-efi/inc/efi.h"
#include "printing.h"

void println(EFI_SYSTEM_TABLE *system_table, CHAR16 *str)
{
    print(system_table, str);
    print(system_table, (CHAR16 *)L"\r\n");
}

void memset(void *buf, char n, size_t size)
{
    char *b = (char *)buf;
    for (int i = 0; i < size; i++)
        b[i] = n;
}

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
    EFI_RUNTIME_SERVICES *runtime_services;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *config_table; 
} kernel_params;


EFI_STATUS EFIAPI efi_main(IN EFI_HANDLE image_handle, IN EFI_SYSTEM_TABLE *system_table)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs_protocol;
    EFI_FILE_HANDLE root;
    EFI_FILE_HANDLE kernel;
    EFI_FILE_INFO *kernel_info;
    UINTN info_size = 0;
    UINTN buf_size = 0;
    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID file_system_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID graphics_protocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;
    EFI_STATUS ret;
    kernel_params params;
    system_table->BootServices->LocateProtocol(&graphics_protocol, NULL, (VOID **)&graphics);
    graphics->SetMode(graphics, 0);
    
    system_table->BootServices->HandleProtocol(image_handle, &loaded_image_guid , (VOID **)&loaded_image);
    system_table->BootServices->HandleProtocol(loaded_image->DeviceHandle, &file_system_guid, (VOID **)&fs_protocol);
    system_table->ConOut->Reset(system_table->ConOut, 1);
    fs_protocol->OpenVolume(fs_protocol, &root);
    ret = root->Open(root, &kernel, L"EFI\\kernel\\kernel.bin", EFI_FILE_MODE_READ, 0);
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"Open failed\r\n");
    kernel->GetInfo(kernel, &file_info_guid, &info_size, NULL);
    ret = system_table->BootServices->AllocatePool(EfiLoaderData, info_size + 1, (VOID **)&kernel_info);
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"Allocating failed %i\r\n", ret);
    ret = kernel->GetInfo(kernel, &file_info_guid, &info_size, kernel_info);
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"Getting info failed %i\r\n", ret);
    UINT8 *buf;
    buf_size = kernel_info->FileSize;
    
    printf(system_table, (CHAR16 *)L"Mode info size %i \r\n", graphics->Mode->SizeOfInfo);
    printf(system_table, (CHAR16 *)L"Height %i Width %i \r\n", graphics->Mode->Info->VerticalResolution, graphics->Mode->Info->PixelsPerScanLine);
    printf(system_table, (CHAR16 *)L"Color %i \r\n", graphics->Mode->Info->PixelFormat);
    printf(system_table, (CHAR16 *)L"file size is %i\r\n", buf_size);
    ret = system_table->BootServices->AllocatePool(EfiLoaderCode, buf_size + 1, (VOID **)&buf);
    printf(system_table, (CHAR16 *)L"ALLOCATED\r\n");
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"Allocating failed %i\r\n", ret);
    ret = kernel->Read(kernel, &buf_size, buf);
    printf(system_table, (CHAR16 *)L"READ\r\n");
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"reading failed %i\r\n", ret);
    ret = kernel->Close(kernel);
    printf(system_table, (CHAR16 *)L"CLOSED\r\n");
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"closing failed %i\r\n", ret);

    params.graphics_mode = *graphics->Mode;
    void EFIAPI (*kernel_main)(kernel_params) = NULL;
    *(VOID **)&kernel_main = buf;
    EFI_INPUT_KEY key = {0};
    CHAR16 buf2[2] = {0};
    printf(system_table, (CHAR16 *)L"Hi UEFI %i %s\r\n", 16, (CHAR16 *)L"AUJSBAUJS");
    UINTN a = 0;
    memset(&params.map, 0, sizeof(struct memory_map));
    system_table->BootServices->GetMemoryMap(&params.map.size, params.map.mem_map, &params.map.key, &params.map.descriptor_size, &params.map.descriptor_version);
    
    params.map.size += params.map.descriptor_size * 2;
    ret = system_table->BootServices->AllocatePool(EfiLoaderData, params.map.size, (VOID **)&params.map.mem_map);
    if (ret != EFI_SUCCESS)
        printf(system_table, (CHAR16 *)L"Allocating failed %i\r\n", ret);
    
    system_table->BootServices->GetMemoryMap(&params.map.size, params.map.mem_map, &params.map.key, &params.map.descriptor_size, &params.map.descriptor_version);
    INT32 usable_mem = 0;
    for (int i = 0; i < params.map.size / params.map.descriptor_size; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)params.map.mem_map + (i * params.map.descriptor_size));
        printf(system_table, (CHAR16 *)L"%i:Attribute %i Page num %i pad %i phy start %i virt start %i type %i \r\n", i, desc->Attribute, desc->NumberOfPages, desc->Pad, desc->PhysicalStart, desc->VirtualStart, desc->Type);
        if (desc->Type == EfiLoaderCode ||
            desc->Type ==   EfiLoaderData || 
            desc->Type ==   EfiBootServicesCode ||
            desc->Type ==   EfiBootServicesData ||
            desc->Type ==   EfiRuntimeServicesCode ||
            desc->Type ==   EfiRuntimeServicesData ||
            desc->Type ==   EfiConventionalMemory ||
            desc->Type ==   EfiPersistentMemory)
        {
            usable_mem += desc->NumberOfPages * 4096;
        }

    }
    
    printf(system_table, (CHAR16 *)L"Total usable memory %i MB\r\n", usable_mem/(1024 * 1024));
    print(system_table, L"\r\n");
    system_table->BootServices->GetMemoryMap(&params.map.size, params.map.mem_map, &params.map.key, &params.map.descriptor_size, &params.map.descriptor_version);
    ret = system_table->BootServices->ExitBootServices(image_handle, params.map.key);
    if (ret != EFI_SUCCESS)
    {
        printf(system_table, (CHAR16 *)L"Exiting booting services failed %i\r\n", ret);
        return EFI_SUCCESS;
    }
    params.config_table = system_table->ConfigurationTable;
    params.NumberOfTableEntries = system_table->NumberOfTableEntries;
    params.runtime_services = system_table->RuntimeServices;
    kernel_main(params);
        buf2[0] = key.UnicodeChar;
        print(system_table, buf2);

    return EFI_SUCCESS;
}
