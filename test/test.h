#ifndef SP_TEST_H_
#define SP_TEST_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#ifdef _MSC_VER
#include <Windows.h>
#define inline __inline
#pragma warning (disable: 4996)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#ifndef _MSC_VER
#include <dirent.h>
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define t(expr) { \
	if (! (expr)) { \
		printf("fail (%s:%d) %s\n", __FILE__, __LINE__, #expr); \
		fflush(NULL); \
		abort(); \
	} \
}

#define test(f) { \
	printf("%s: ", #f); \
	fflush(NULL); \
	f(); \
	printf("ok\n"); \
	fflush(NULL); \
}

static inline int
exists(char *path, uint32_t epoch, char *ext) {
	char file[1024];
#ifndef _MSC_VER
	snprintf(file, sizeof(file), "%s/%"PRIu32".%s", path, epoch, ext);
	struct stat st;
	return lstat(file, &st) == 0;
#else
    _snprintf(file, sizeof(file), "%s/%"PRIu32".%s", path, epoch, ext);
    struct _stat st;
    return _stat(file, &st) == 0;
#endif
}

static inline int rmrf(char *path) {
#ifndef _MSC_VER
	DIR *d = opendir(path);
	if (d == NULL)
		return -1;
	char file[1024];
	struct dirent *de;
	while ((de = readdir(d))) {
		if (de->d_name[0] == '.')
			continue;
		snprintf(file, sizeof(file), "%s/%s", path, de->d_name);
		int rc = unlink(file);
		if (rc == -1) {
			closedir(d);
			return -1;
		}
	}
	closedir(d);
	return rmdir(path);
#else
    int len = strlen(path);
    char * filter = _alloca(len + 3);
    strcpy(filter, path);
    if (filter[len - 1] == '/')
        strcat(filter, "*");
    else
        strcat(filter, "/*");
    WIN32_FIND_DATAA fd;
    HANDLE find = FindFirstFileA(filter, &fd);
    if (find != INVALID_HANDLE_VALUE) {
        char file[1024];
        do {
            _snprintf(file, sizeof(file), "%s/%s", path, fd.cFileName);
            if (fd.cFileName[0] == '.')
                continue;

            int rc = unlink(file);
            if (rc == -1) {
                FindClose(find);
                return -1;
            }
        } while (FindNextFileA(find, &fd));
        FindClose(find);
    }
    return rmdir(path);
#endif
}

#endif
