#pragma once
#include "../value.hpp"
#include "../char-array.hpp"

namespace Mirb
{
	namespace Kernel
	{
		value_t at_exit(value_t block);
		value_t exit(intptr_t result);
		value_t proc(value_t block);
		value_t benchmark(value_t block);
		value_t backtrace();
		value_t block_given();
		value_t rb_loop(value_t value);
		value_t eval(value_t obj, String *input);
		value_t load(String *filename);
		value_t require(String *filename);
		value_t require_relative(String *filename);
		value_t print(size_t argc, value_t argv[]);
		value_t puts(size_t argc, value_t argv[]);
		value_t raise(value_t first, String *str);
		value_t rb_cast_array(value_t value);
		value_t rb_cast_string(value_t value);
		value_t rb_cast_integer(value_t value);
		
		void read_file(CharArray filename, bool try_relative, bool require, CharArray &full_path, bool& loaded, char_t *&data, size_t &length);

		void initialize();
	};
};
