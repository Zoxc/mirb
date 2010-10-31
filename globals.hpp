#pragma once

#ifdef _MSC_VER
	#define WIN32 1
	#define MSVC 1
	#pragma warning(disable:4355)
	#pragma warning(disable:4996)
	#define mirb_external(name)
	#define __thread __declspec(thread)
	#define __noreturn __declspec(noreturn)
#else
	#define __noreturn __attribute__((noreturn)) 
	#define mirb_external(name) __asm__(name)
#endif

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cassert>

#define RT_ALIGN(value, to) ((value + (to) - 1) & ~((to) - 1))

#ifdef VALGRIND
	#ifndef NO_GC
		#define NO_GC
	#endif
#endif

#if defined(DEBUG) && !defined(MSVC)
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
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <excpt.h>
#else
	#define __stdcall __attribute__((__stdcall__))
	#define __cdecl __attribute__((__cdecl__))

	#include <sys/mman.h>
	#include <sys/stat.h>
#endif

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#if !__has_builtin(__builtin_unreachable)
	static inline void __noreturn __builtin_unreachable();
	static inline void __builtin_unreachable()
	{
		RT_ASSERT(0);
	}
#endif

#include "vector.hpp"
#include "hash.hpp"
