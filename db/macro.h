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

#else
#include <stddef.h>
#define sphot
#define splikely(EXPR) (EXPR)
#define spunlikely(EXPR) (EXPR)
#define spcast(N, T, F) ((T*)((char*)(N) - offsetof(T, F)))
#define sppacked
#define spunused


static inline __declspec(naked)
char __sync_lock_test_and_set(volatile char * a, char v) {
    // Here's a quick mess to implement 8bit InterlockedExchange which
    // apparently is only available in windows8 - this is very bad
    _asm {
            mov         al, byte ptr [esp+8]
            mov         ecx, dword ptr[esp+4]
            xchg        al, byte ptr[ecx]
            ret
    };
}
static inline __declspec(naked)
void __sync_lock_release(volatile char * a)
{
    // Here's a quick mess to implement 8bit InterlockedExchange which
    // apparently is only available in windows8 - this is very bad
    _asm {
        mov         al, 0
        mov         ecx, dword ptr[esp+4]
        xchg        al, byte ptr[ecx]
        ret
    };
}

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

_Check_return_ _CRTIMP int __cdecl _mkdir(_In_z_ const char * _Path);

/*
static inline int mkdir(const char * _Path, int perms) {
    return _mkdir(_Path);
}
*/
static inline int ftruncate(int fd, long size) {
    return _chsize(fd, size);
}
#define strdup _strdup
#define PROT_READ FILE_MAP_READ
#define PROT_WRITE FILE_MAP_WRITE

#endif

#define spdiv(a, b) ((a) + (b) - 1) / (b)

#endif