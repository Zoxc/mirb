#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define RT_ALIGN(value, to) ((value + (to) - 1) & ~((to) - 1))

#ifdef DEBUG
	#define GC_DEBUG
	#define RT_ASSERT(condition) do { \
		if(!(condition)) { \
			__asm__("int3\n"); \
			assert(0); \
		} \
	} while(0)
#else
	#define RT_ASSERT(condition) assert(condition)
#endif

#define __regparm(n) __attribute__((__regparm__(n)))

#ifdef WIN32
    #include <windows.h>
    #include <excpt.h>
    #include "vendor/BeaEngine/BeaEngine.h"
#else
	#define __stdcall __attribute__((__stdcall__))
	#define __cdecl __attribute__((__cdecl__))

	#include <sys/mman.h>
	#include <sys/stat.h>
#endif

static inline void __attribute__((noreturn)) __builtin_unreachable(void)
{
	while(1);
}

#define GC_THREADS

#include "vendor/gc/include/gc.h"
#include "vendor/kvec_cm.h"
#include "vendor/kvec.h"
#include "vendor/khash.h"
