#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Kernel
	{
		value_t proc(value_t block);
		value_t benchmark(value_t block);
		value_t eval(value_t obj, value_t code);
		value_t load(value_t obj, value_t filename);
		value_t print(size_t argc, value_t argv[]);
		value_t puts(size_t argc, value_t argv[]);
		value_t raise(size_t argc, value_t argv[]);

		extern value_t class_ref;
		
		void initialize();
	};
};
