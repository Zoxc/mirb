#pragma once

#ifdef DEBUG
	#define GC_DEBUG
#endif

#define GC_THREADS

extern "C"
{
    #include <gc/include/gc.h>
};
