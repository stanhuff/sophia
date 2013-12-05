#ifndef SP_MACRO_H_
#define SP_MACRO_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#ifndef _MSC_VER
#define sppacked __attribute__((packed))
#define spunused __attribute__((unused))

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
#  define sphot __attribute__((hot))
#else
#  define sphot
#endif

#define splikely(EXPR) __builtin_expect(!! (EXPR), 1)
#define spunlikely(EXPR) __builtin_expect(!! (EXPR), 0)
#define spcast(N, T, F) ((T*)((char*)(N) - __builtin_offsetof(T, F)))
#define spdiv(a, b) ((a) + (b) - 1) / (b)

#else
#include <stddef.h>
#define sphot
#define splikely(EXPR) (EXPR)
#define spunlikely(EXPR) (EXPR)
#define spcast(N, T, F) ((T*)((char*)(N) - offsetof(T, F)))
#define spdiv(a, b) ((a) + (b) - 1) / (b)
#define sppacked
#define spunused

#define usleep(x) Sleep((x)*1000)

#ifdef _WIN64
typedef signed __int64    ssize_t;
#else  /* _WIN64 */
typedef _W64 signed int   ssize_t;
#endif  /* _WIN64 */

struct iovec {
    void*    iov_base;  /* base address of the data storage area */
    /* represented by the iovec structure */
    size_t    iov_len;   /* size of the data storage area in bytes */
};
#define snprintf _snprintf

static inline int ftruncate(int fd, long size) {
    return _chsize(fd, size);
}
#define strdup _strdup
#define PROT_READ FILE_MAP_READ
#define PROT_WRITE FILE_MAP_WRITE

#endif

#endif
