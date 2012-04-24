#pragma once
#ifdef _MSC_VER
	#pragma warning(disable:4355)
	#pragma warning(disable:4996)
	#pragma warning(disable:4530)
	//#pragma warning(disable:4731)
	//#pragma warning(disable:4200) // TODO: Remove this
#endif

#include <Prelude/Internal/Common.hpp>

#ifdef WIN32
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <excpt.h>
#else
	#include <sys/mman.h>
	#include <sys/stat.h>
#endif

#include <type_traits> 
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#ifdef VALGRIND
	#ifndef NO_GC
		#define NO_GC
	#endif
#endif

namespace Mirb
{
	using namespace Prelude;

	typedef uint8_t char_t;
	
	#define mirb_runtime_assert(expression) prelude_runtime_assert(expression)
	#define mirb_runtime_abort(message) prelude_runtime_abort(message)

	#define mirb_debug_assert(expression) prelude_debug_assert(expression)
	#define mirb_debug_abort(message) prelude_debug_abort(message)
};
