#include <stdarg.h>
#include <unistd.h>

void putchar(char c)
{
    char buf[2] = {0, 0};
    buf[0] = c;
    write(1, buf, 1);
}

void print(char *str)
{
    while (*str)
    {
        putchar(*str);
        str++;
    }
}

void putnumber(int num)
{
    if (num == -2147483648)
		return print("-2147483648");
    if (num < 0)
    {
        putchar('-');
        num = num * -1;
    }
    if (num >= 10)
    {
        putnumber(num);
        num = num % 10;
    }
    if (num < 10)
        putchar(num + '0');
}

void printf(const char *format, ...)
{
    va_list pointer;

    va_start(pointer, format);
    while (*format)
    {
        if (!format)
            break;
        if (*format == '%')
        {
            format++;
            if (!*format)
                break;
            switch (*format)
            {
                case 'i':
                    putnumber((int)va_arg(pointer, int));
                    break;
            }
        }
        else 
            putchar(*format);
        format++;
    }
}

int main()
{
    printf("Hi UEFI %i", 16);
}