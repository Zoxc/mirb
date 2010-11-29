#include "kernel.hpp"
#include "../classes/object.hpp"
#include "../classes/string.hpp"
#include "../classes/symbol.hpp"
#include "../classes/exception.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"
#include "../arch/support.hpp"

namespace Mirb
{
	value_t Kernel::class_ref;
	
	mirb_compiled_block(kernel_proc)
	{
		if(block)
			return block;
		else
			return value_nil;
	}

	mirb_compiled_block(kernel_eval)
	{
		value_t arg = MIRB_ARG(0);

		if(Value::type(arg) != Value::String)
			return value_nil;

		String *input = cast<String>(arg);

		CharArray c_str = input->string.c_str();
		CharArray filename("eval");

		return eval(obj, method_name, method_module, c_str.str_ref(), c_str.str_length(), filename);
	}

	static FILE *open_file(CharArray &filename)
	{
		CharArray filename_c_str = filename.c_str();

		#ifndef WIN32
			struct stat buf;
		#endif

		bool is_dir = false;
		FILE* file = 0;

		#ifndef WIN32
			if(stat(filename_c_str.c_str_ref(), &buf) != -1)
				is_dir = S_ISDIR(buf.st_mode);
		#endif

		if(!is_dir)
			file = fopen(filename_c_str.c_str_ref(), "rb");

		if(is_dir || !file)
		{
			is_dir = false;

			CharArray filename_rb = (filename + ".rb").c_str();
			
			#ifndef WIN32
				if(stat(filename_rb.c_str_ref(), &buf) != -1)
					is_dir = S_ISDIR(buf.st_mode);
			#endif

			if(!is_dir)
				file = fopen(filename_rb.c_str_ref(), "rb");

			if(is_dir || !file)
				return 0;

			filename = filename_rb;

			return file;
		}

		return file;
	}

	static value_t run_file(value_t self, CharArray filename)
	{
		FILE* file = open_file(filename);

		if(!file)
			return value_nil;

		fseek(file, 0, SEEK_END);

		size_t length = ftell(file);

		fseek(file, 0, SEEK_SET);

		char *data = (char *)malloc(length + 1);

		runtime_assert(data);

		if(fread(data, 1, length, file) != length)
		{
			free(data);
			fclose(file);

			return value_nil;
		}

		data[length] = 0;

		value_t result = eval(self, Symbol::from_char_array("in " + filename), class_of(main), (char_t *)data, length, filename);

		free(data);
		fclose(file);

		return result;
	}

	mirb_compiled_block(kernel_load)
	{
		value_t filename = MIRB_ARG(0);

		return run_file(obj, cast<String>(filename)->string);
	}

	mirb_compiled_block(kernel_print)
	{
		MIRB_ARG_EACH(i)
		{
			value_t arg = argv[i];

			if(Value::type(arg) != Value::String)
				arg = call(arg, "to_s");
			
			if(Value::type(arg) == Value::String)
				std::cout << cast<String>(arg)->string.get_string();
		}
		
		return value_nil;
	}

	mirb_compiled_block(kernel_raise)
	{
		value_t instance_of = Exception::class_ref;
		value_t message;
		value_t backtrace;

		size_t i = 0;

		if(Value::type(MIRB_ARG(0)) == Value::Class)
		{
			instance_of = MIRB_ARG(0);
			i++;
		}
		else
			instance_of = Exception::class_ref;
	
		if(argc > i)
		{
			message = MIRB_ARG(i);
			i++;
		}
		else
			message = value_nil;

		if(argc > i)
		{
			backtrace = MIRB_ARG(i);
			i++;
		}
		else
			backtrace = value_nil;

		Exception *exception = new (gc) Exception(instance_of, message, backtrace);

		ExceptionData data;

		data.type = Mirb::RubyException;
		data.target = 0;
		data.value = auto_cast(exception);

		Arch::Support::exception_raise(&data);
	}

	void Kernel::initialize()
	{
		Kernel::class_ref = define_module(Object::class_ref, "Kernel");

		include_module(Object::class_ref, Kernel::class_ref);

		define_method(Kernel::class_ref, "proc", kernel_proc);
		define_method(Kernel::class_ref, "eval", kernel_eval);
		define_method(Kernel::class_ref, "print", kernel_print);
		define_method(Kernel::class_ref, "load", kernel_load);
		define_method(Kernel::class_ref, "require", kernel_load);
		define_method(Kernel::class_ref, "require_relative", kernel_load);
		define_method(Kernel::class_ref, "raise", kernel_raise);
	}
};
