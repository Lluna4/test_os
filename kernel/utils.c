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
