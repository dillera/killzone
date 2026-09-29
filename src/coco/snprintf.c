#include <cmoc.h>
#include <stdarg.h>
#include "snprintf.h"

/* cmoc has no snprintf. Output beyond 127 characters overruns scratch. */
int snprintf(char *buf, size_t n, const char *fmt, ...)
{
    static char scratch[128];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsprintf(scratch, fmt, ap);
    va_end(ap);

    if (n) {
        strncpy(buf, scratch, n - 1);
        buf[n - 1] = '\0';
    }
    return len;
}
