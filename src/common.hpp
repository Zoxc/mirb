#pragma once
#include "../globals.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#ifdef WIN32
	#include <windows.h>
#endif

namespace Mirb
{
	typedef uint8_t char_t;

	static const size_t memory_align = 8;

	static inline size_t align(size_t value, size_t alignment)
	{
		alignment -= 1;
		return (value + alignment) & ~alignment;
	};

	static inline size_t align_down(size_t value, size_t alignment)
	{
		return value & ~(alignment - 1);
	};
	
	static inline void __noreturn debug_fail(std::string message = "");

	static inline void debug_assert(bool expr, std::string message)
	{
		assert(expr);
	}

	static inline void debug_fail(std::string message)
	{
		assert(0);
	}
};
