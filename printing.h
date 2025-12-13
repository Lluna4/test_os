#include <stdarg.h>
#include "/usr/include/efi/efi.h"

void putchar(EFI_SYSTEM_TABLE *system_table, CHAR16 c)
{
    CHAR16 buf[2] = {0, 0};
    buf[0] = c;
    system_table->ConOut->OutputString(system_table->ConOut, buf);
}

void print(EFI_SYSTEM_TABLE *system_table, CHAR16 *str)
{
    system_table->ConOut->OutputString(system_table->ConOut, str);
}

void putnumber(INT64 num, EFI_SYSTEM_TABLE *system_table)
{
    if (num == -2147483648)
		return (print(system_table, (CHAR16 *)L"-2147483648"));
    if (num < 0)
    {
        putchar(system_table,L'-');
        num = num * -1;
    }
    if (num >= 10)
    {
        putnumber(num/10, system_table);
        num = num % 10;
    }
    if (num < 10)
        putchar(system_table, num + L'0');
}

void printf(EFI_SYSTEM_TABLE *system_table, const CHAR16 *format, ...)
{
    va_list pointer;

    va_start(pointer, format);
    while (*format)
    {
        if (!format)
            break;
        if (*format == L'%')
        {
            format++;
            if (!*format)
                break;
            switch (*format)
            {
                case L'i':
                    putnumber((unsigned int)va_arg(pointer, unsigned int), system_table);
                    break;
                case L's':
                    print(system_table, (CHAR16 *)va_arg(pointer, CHAR16 *));
                    break;
            }
        }
        else 
            putchar(system_table, *format);
        format++;
    }
}