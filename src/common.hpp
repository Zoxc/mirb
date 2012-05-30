#pragma once
#ifdef _MSC_VER
	#pragma warning(disable:4355)
	#pragma warning(disable:4996)
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

#include <cstdarg>
#include <algorithm> 
#include <type_traits> 
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

namespace Mirb
{
	using namespace Prelude;

	typedef uint8_t char_t;
	typedef char_t char_code_t;

	#ifdef DEBUG
		#define mirb_debug(expression)  expression
	#else
		#define mirb_debug(expression)
	#endif

	#define mirb_runtime_assert(expression) prelude_runtime_assert(expression)
	#define mirb_runtime_abort(message) prelude_runtime_abort(message)

	#define mirb_debug_assert(expression) prelude_debug_assert(expression)
	#define mirb_debug_abort(message) prelude_debug_abort(message)
	
	/*
	 *	Robert Jenkins' 32 bit integer hash
	 */
	inline uint32_t hash_number(uint32_t a)
	{
		a = (a+0x7ed55d16) + (a<<12);
		a = (a^0xc761c23c) ^ (a>>19);
		a = (a+0x165667b1) + (a<<5);
		a = (a+0xd3a2646c) ^ (a<<9);
		a = (a+0xfd7046c5) + (a<<3);
		a = (a^0xb55a4f09) ^ (a>>16);

		return a;
	}
};
