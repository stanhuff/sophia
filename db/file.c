
/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
 */

#ifndef _MSC_VER
#include <sys/mman.h>
#include <unistd.h>
#else
#pragma warning( disable: 4996 ) // POSIX name is deprecated
#include <sp.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <sp.h>

int sp_fileexists(char *path) {
#ifdef _MSC_VER
    WIN32_FIND_DATAA fd;
    HANDLE file = FindFirstFileA(path, &fd);
    if (file != INVALID_HANDLE_VALUE)
        FindClose(file);
    return file != INVALID_HANDLE_VALUE;
#else
    struct stat st;
    int rc = lstat(path, &st);
    return rc == 0;
#endif
}

int sp_filerm(char *path) {
    return unlink(path);
}

static inline ssize_t
sp_mapsizeof(char *path) {
#ifdef _MSC_VER
    struct _stat st;
    int rc = _stat(path, &st);
#else
    struct stat st;
    int rc = lstat(path, &st);
#endif
    if (spunlikely(rc == -1))
        return -1;
    return st.st_size;
}

static inline int
sp_mapresize(spfile *f, size_t size) {
    int rc = ftruncate(f->fd, size);
    if (spunlikely(rc == -1))
        return -1;
    f->size = size;
    return 0;
}

static inline int
sp_map(spfile *f, int flags) {
#ifdef _MSC_VER
    int pageAccess;
    if ((flags & (PROT_READ | PROT_WRITE)) == (PROT_READ | PROT_WRITE))
        pageAccess = PAGE_READWRITE;
    else
        pageAccess = PAGE_READONLY;


    HANDLE mapping = CreateFileMapping((HANDLE)_get_osfhandle(f->fd), NULL, pageAccess, 0, 0, NULL);
    if (mapping == NULL)
        return -1;

    void *p = MapViewOfFile(mapping, flags, 0, 0, 0);
    if (p == 0) {
        CloseHandle(mapping);
        return -1;
    }
    f->mapping = mapping;
    f->map = p;
    return 0;
#else
    void *p = mmap(NULL, f->size, flags, MAP_SHARED, f->fd, 0);
    if (p == MAP_FAILED)
        return -1;
    f->map = p;
    return 0;
#endif
}

static inline int
sp_unmap(spfile *f) {
    int rc;
    if (f->map) {
#ifdef _MSC_VER
        rc = UnmapViewOfFile(f->map);
        f->map = NULL;
        CloseHandle(f->mapping);
        f->mapping = INVALID_HANDLE_VALUE;
        return !rc;
#else
        rc = munmap(f->map, f->size);
        f->map = NULL;
        return rc;
#endif
    }
    return 0;
}

static inline int
sp_mapopenof(spfile *f, char *path, int flags, uint64_t size)
{
#if _MSC_VER
    f->fd = sp_win_openfile(path, flags);
#else
    f->fd = open(path, flags, 0600);
#endif
    if (spunlikely(f->fd == -1))
        return -1;
    f->file = sp_strdup(f->a, path);
    if (spunlikely(f->file == NULL)) {
        close(f->fd);
        f->fd = -1;
        return -1;
    }
    f->used = 0;
    f->creat = (flags & O_CREAT ? 1 : 0);
    int rc;
    if (!f->creat) {
        ssize_t sz = sp_mapsizeof(path);
        if (spunlikely(sz == -1))
            goto err;
        f->size = sz;
        rc = sp_map(f, PROT_READ);
        if (spunlikely(rc == -1))
            goto err;
        return 0;
    }
    f->size = 0;
    rc = sp_mapresize(f, size);
    if (spunlikely(rc == -1))
        goto err;
    rc = sp_map(f, PROT_READ | PROT_WRITE);
    if (spunlikely(rc == -1))
        goto err;
    return 0;
err:
    close(f->fd);
    f->fd = -1;
    sp_free(f->a, f->file);
    f->file = NULL;
    return -1;
}

int sp_mapopen(spfile *f, char *path) {
    return sp_mapopenof(f, path, O_RDONLY, 0);
}

int sp_mapnew(spfile *f, char *path, uint64_t size) {
    return sp_mapopenof(f, path, O_RDWR | O_CREAT, size);
}

static inline int
sp_mapsync(spfile *f) {
#ifdef _MSC_VER
    if (!FlushViewOfFile(f->map, f->size))
        return -1;
    if (!FlushFileBuffers((HANDLE)_get_osfhandle(f->fd)))
        return -1;
    return 0;
#else
    return msync(f->map, f->size, MS_SYNC);
#endif
}

static inline int
sp_mapcut(spfile *f)
{
    if (f->creat == 0)
        return 0;
    int rc = sp_mapsync(f);
    if (spunlikely(rc == -1))
        return -1;
    rc = sp_unmap(f);
    if (spunlikely(rc == -1))
        return -1;
    rc = sp_mapresize(f, f->used);
    if (spunlikely(rc == -1))
        return -1;
    return 0;
}

int sp_mapunlink(spfile *f) {
#ifdef _MSC_VER
    if (f->map) {
        sp_mapcut(f);
        if (f->map)
            sp_unmap(f);
    }
#endif
    return sp_filerm(f->file);
}

static inline int
sp_fileclose(spfile *f)
{
    /* leave file incomplete */
    if (splikely(f->file)) {
        sp_free(f->a, f->file);
        f->file = NULL;
    }
    int rc;
    if (spunlikely(f->fd != -1)) {
        rc = close(f->fd);
        if (spunlikely(rc == -1))
            return -1;
        f->fd = -1;
    }
    return 0;
}

static inline int
sp_filecomplete(spfile *f)
{
    if (f->creat == 0)
        return 0;
    /* remove .incomplete part */
    f->creat = 0;
    char path[1024];
    snprintf(path, sizeof(path), "%s", f->file);
    char *ext = strrchr(path, '.');
    assert(ext != NULL);
    *ext = 0;
    int rc = rename(f->file, path);
    if (spunlikely(rc == -1))
        return -1;
    char *p = sp_strdup(f->a, path);
    if (spunlikely(p == NULL))
        return -1;
    sp_free(f->a, f->file);
    f->file = p;
    return 0;
}

int sp_mapunmap(spfile *f) {
    return sp_unmap(f);
}

int sp_mapclose(spfile *f)
{
    if (f->map) {
        int rc = sp_mapcut(f);
        if (spunlikely(rc == -1))
            return -1;
        if (f->map) {
            int rc = sp_unmap(f);
            if (spunlikely(rc == -1))
                return -1;
        }
    }
    return sp_fileclose(f);
}

int sp_mapcomplete(spfile *f)
{
    if (f->creat == 0)
        return 0;
    /* sync and truncate file by used size */
    int rc = sp_mapcut(f);
    if (spunlikely(rc == -1))
        return -1;
    /* remove .incomplete part */
    rc = sp_filecomplete(f);
    if (spunlikely(rc == -1))
        return -1;
    return sp_map(f, PROT_READ);
}

int sp_mapensure(spfile *f, uint64_t size, float grow)
{
    if (splikely((f->used + size) < f->size))
        return 0;
    int rc = sp_unmap(f);
    if (spunlikely(rc == -1))
        return -1;
    long double nsz = f->size * grow;
    if (spunlikely(nsz < size))
        nsz = size;
    rc = sp_mapresize(f, (size_t)nsz);
    if (spunlikely(rc == -1))
        return -1;
    return sp_map(f, PROT_READ | PROT_WRITE);
}

static inline int
sp_logopenof(spfile *f, char *path, int flags)
{
#if _MSC_VER
    f->fd = sp_win_openfile(path, flags);
#else
    f->fd = open(path, flags, 0600);
#endif
    f->creat = 1;
    if (spunlikely(f->fd == -1))
        return -1;
    f->file = sp_strdup(f->a, path);
    if (spunlikely(f->file == NULL)) {
        close(f->fd);
        f->fd = -1;
        return -1;
    }
    f->size = 0;
    f->used = 0;
    return 0;
}

int sp_lognew(spfile *f, char *dir, uint32_t epoch)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%"PRIu32".log.incomplete",
        dir, epoch);
    int rc = sp_logopenof(f, path, O_WRONLY | O_APPEND | O_CREAT);
    if (spunlikely(rc == -1))
        return -1;
    // posix_fadvise(seq)
    return 0;
}

int sp_logcontinue(spfile *f, char *dir, uint32_t epoch) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%"PRIu32".log.incomplete",
        dir, epoch);
    int rc = sp_logopenof(f, path, O_WRONLY | O_APPEND);
    if (spunlikely(rc == -1))
        return -1;
    return 0;
}

int sp_logclose(spfile *f) {
    return sp_fileclose(f);
}


static inline int
sp_logsync(spfile *f) {
#ifdef _MSC_VER
    return (f->creat ? !FlushFileBuffers((HANDLE)_get_osfhandle(f->fd)) : 0);
#else
    return (f->creat ? fsync(f->fd) : 0);
#endif
}

int sp_logcomplete(spfile *f)
{
    int rc = sp_logsync(f);
    if (spunlikely(rc == -1))
        return -1;
    return sp_filecomplete(f);
}

int sp_logcompleteforce(spfile *f) {
    int rc = sp_logsync(f);
    if (spunlikely(rc == -1))
        return -1;
    int oldcreat = f->creat;
    f->creat = 1;
    rc = sp_filecomplete(f);
    f->creat = oldcreat;
    return rc;
}

int sp_logunlink(spfile *f) {
    return sp_filerm(f->file);
}

int sp_logwrite(spfile *f, void *buf, size_t size)
{
    size_t n = 0;
    do {
        ssize_t r;
        do {
            r = write(f->fd, (char*)buf + n, size - n);
        } while (r == -1 && errno == EINTR);
        if (r <= 0)
            return -1;
        n += r;
    } while (n != size);
    f->used += size;
    return 0;
}

int sp_logput(spfile *f, spbatch *b)
{
    register struct iovec *v = b->iov;
    register uint64_t size = 0;
    register int n = b->iovc;
    do {
        int r;
#ifdef _MSC_VER
        HANDLE h = (HANDLE)_get_osfhandle(f->fd);
        r = 0;
        for (int i = 0; i < n; i++) {
            DWORD written;
            if (!WriteFile(h, v[i].iov_base, v[i].iov_len, &written, NULL)) {
                b->iovc = 0;
                return -1;
            }
            r += written;
        }
#else
        do {
            r = writev(f->fd, v, n);
        } while (r == -1 && errno == EINTR);
        if (r < 0) {
            b->iovc = 0;
            return -1;
        }
#endif
        size += r;
        while (n > 0) {
            if (v->iov_len > (size_t)r) {
                v->iov_base = (char*)v->iov_base + r;
                v->iov_len -= r;
                break;
            }
            else {
                r -= v->iov_len;
                v++;
                n--;
            }
        }
    } while (n > 0);
    f->used += size;
    b->iovc = 0;
    return 0;
}

int sp_logrlb(spfile *f)
{
    int rc = ftruncate(f->fd, f->svp);
    if (spunlikely(rc == -1))
        return -1;
    f->used = f->svp;
    f->svp = 0;
    return lseek(f->fd, f->used, SEEK_SET);
}

int sp_logeof(spfile *f)
{
    sp_filesvp(f);
    speofh eof = { SPEOF };
    int rc = sp_logwrite(f, &eof, sizeof(eof));
    if (spunlikely(rc == -1)) {
        sp_logrlb(f);
        return -1;
    }
    return 0;
}

int sp_lockfile(spfile *f, char *path)
{
    f->creat = 0;
#ifdef _MSC_VER
    f->fd = sp_win_openfileshare(path, O_CREAT | O_WRONLY, 0);
    if (f->fd == -1 && GetLastError() == ERROR_SHARING_VIOLATION)
        return 1;   // already locked
#else
    f->fd = open(path, O_CREAT | O_WRONLY, 0600);
#endif
    if (spunlikely(f->fd == -1))
        return -1;
    f->file = sp_strdup(f->a, path);
    if (spunlikely(f->file == NULL)) {
        close(f->fd);
        f->fd = -1;
        return -1;
    }
#ifdef _MSC_VER
    int rc = LockFile((HANDLE)_get_osfhandle(f->fd), 0, 0, 0xffffffff, 0xffffffff);
    if (spunlikely(rc == 0)) {
        sp_fileclose(f);
        return 1;
    }
    return 0;
#else
    struct flock l;
    memset(&l, 0, sizeof(l));
    l.l_whence = SEEK_SET;
    l.l_start = 0;
    l.l_len = 0;
    l.l_type = F_WRLCK;
    int rc = fcntl(f->fd, F_SETLK, &l);
    if (spunlikely(rc == -1)) {
        sp_fileclose(f);
        return 1;
    }
    return 0;
#endif
}

int sp_unlockfile(spfile *f)
{
    if (spunlikely(f->fd == -1))
        return 0;

#ifdef _MSC_VER
    UnlockFile((HANDLE)_get_osfhandle(f->fd), 0, 0, 0xffffffff, 0xffffffff);
#else
    struct flock l;
    memset(&l, 0, sizeof(l));
    l.l_whence = SEEK_SET;
    l.l_start = 0;
    l.l_len = 0;
    l.l_type = F_UNLCK;
    fcntl(f->fd, F_SETLK, &l);
#endif
    unlink(f->file);
    return sp_fileclose(f);
}
