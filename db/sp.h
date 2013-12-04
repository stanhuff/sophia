#ifndef SP_H_
#define SP_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#ifdef _MSC_VER
#define inline __inline
#include <Windows.h>
#else
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#ifdef _MSC_VER
#include <winsupport.h>
#else
#include <pthread.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <sys/uio.h>
#endif
#include <errno.h>

#include <sophia.h>

#include <macro.h>
#include <crc.h>
#include <lock.h>
#include <list.h>
#include <e.h>
#include <a.h>
#include <meta.h>
#include <file.h>
#include <ref.h>
#include <i.h>
#include <rep.h>
#include <cat.h>
#include <task.h>
#include <core.h>
#include <util.h>
#include <recover.h>
#include <merge.h>
#include <gc.h>
#include <cursor.h>

#endif
