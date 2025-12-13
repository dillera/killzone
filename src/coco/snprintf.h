#ifndef SNPRINTF_H
#define SNPRINTF_H

#include <cmoc.h>
#include <coco.h>

#define snprintf(buf, n, fmt, ...) sprintf((buf), (fmt), __VA_ARGS__)

#endif /* SNPRINTF_H */