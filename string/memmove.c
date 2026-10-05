#include <string.h>

void *
memmove(void *dst0, const void *src0, size_t length)
{
    if (length == 0 || dst0 == src0)
        return dst0;

    bcopy(src0, dst0, length);
    return dst0;
}
