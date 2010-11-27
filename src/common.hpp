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
	
	static inline void __noreturn runtime_fail(std::string message = "");

	template<typename T> static inline void runtime_assert(T expr, std::string message = "")
	{
		if(!expr && message != "")
			std::cout << message << "\n";

		assert(expr);
	}

	static inline void runtime_fail(std::string message)
	{
		std::cout << message << "\n";

		assert(0);
	}
	
	static inline void __noreturn debug_fail(std::string message = "");

	template<typename T> static inline void debug_assert(T expr, std::string message = "")
	{
		runtime_assert(expr, message);
	}

	static inline void debug_fail(std::string message)
	{
		runtime_fail(message);
	}
};
