#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WINDOWS
    #include <windows.h>
    #include "BeaEngine.h"
#else
	#define __stdcall __attribute__((__stdcall__))
	#define __cdecl __attribute__((__cdecl__))

	#include <sys/mman.h>
#endif

#include "vendor/kvec.h"
#include "vendor/khash.h"
