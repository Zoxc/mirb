#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Kernel
	{
		value_t at_exit(value_t block);
		value_t proc(value_t block);
		value_t benchmark(value_t block);
		value_t backtrace();
		value_t block_given();
		value_t eval(value_t obj, value_t code);
		value_t load(value_t filename);
		value_t require(value_t filename);
		value_t require_relative(value_t filename);
		value_t print(size_t argc, value_t argv[]);
		value_t puts(size_t argc, value_t argv[]);
		value_t raise(size_t argc, value_t argv[]);

		void initialize();
	};
};
