#include "kernel.hpp"
#include "../classes/object.hpp"
#include "../classes/string.hpp"
#include "../classes/symbol.hpp"
#include "../classes/exception.hpp"
#include "../classes/exceptions.hpp"
#include "../generic/benchmark.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Kernel::class_ref;
	
	value_t Kernel::proc(value_t block)
	{
		if(block)
			return block;
		else
			return value_nil;
	}
	
	value_t Kernel::benchmark(value_t block)
	{
		CharArray result = Mirb::benchmark([&] {
			yield(block);
		}).format();

		return result.to_string();
	}
	
	value_t Kernel::eval(value_t obj, value_t code)
	{
		if(Value::type(code) != Value::String)
			return value_nil;

		String *input = cast<String>(code);

		CharArray c_str = input->string.c_str();
		CharArray filename("(eval)");

		return eval(obj, 0, value_nil, c_str.str_ref(), c_str.str_length(), filename);
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

		mirb_runtime_assert(data != 0);

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
	
	value_t Kernel::load(value_t obj, value_t filename)
	{
		return run_file(obj, cast<String>(filename)->string);
	}
	
	value_t Kernel::print(size_t argc, value_t argv[])
	{
		for(size_t i = 0; i < argc; ++i)
		{
			value_t arg = argv[i];

			if(Value::type(arg) != Value::String)
				arg = call(arg, "to_s");
			
			if(Value::type(arg) == Value::String)
				std::cout << cast<String>(arg)->string.get_string();
		}
		
		return value_nil;
	}
	
	value_t Kernel::puts(size_t argc, value_t argv[])
	{
		for(size_t i = 0; i < argc; ++i)
		{
			value_t arg = argv[i];

			if(Value::type(arg) != Value::String)
				arg = call(arg, "to_s");
			
			if(Value::type(arg) == Value::String)
				std::cout << cast<String>(arg)->string.get_string();

			std::cout << "\n";
		}
		
		return value_nil;
	}
	
	value_t Kernel::raise(size_t argc, value_t argv[])
	{
		value_t instance_of;
		value_t message;
		value_t backtrace;

		size_t i = 0;

		if((argc > i) && (Value::type(argv[i]) == Value::Class))
		{
			instance_of = argv[i];
			i++;
		}
		else
			instance_of = RuntimeError::class_ref;
	
		if(argc > i)
		{
			message = argv[i];
			i++;
		}
		else
			message = value_nil;

		if(argc > i)
		{
			backtrace = argv[i];
			i++;
		}
		else
		{
			OnStack<2> os(instance_of, message);
			backtrace = Mirb::backtrace().to_string();
		}

		Exception *exception = Collector::allocate<Exception>(instance_of, message, backtrace);

		return raise(auto_cast(exception));
	}

	value_t Kernel::backtrace()
	{
		return Mirb::backtrace().to_string();
	}

	void Kernel::initialize()
	{
		Kernel::class_ref = define_module(Object::class_ref, "Kernel");

		include_module(Object::class_ref, Kernel::class_ref);
		
		static_method<Arg::Block>(Kernel::class_ref, "proc", &proc);
		static_method<Arg::Block>(Kernel::class_ref, "benchmark", &benchmark);
		static_method(Kernel::class_ref, "backtrace", &backtrace);
		static_method<Arg::Self, Arg::Value>(Kernel::class_ref, "eval", &eval);
		static_method<Arg::Count, Arg::Values>(Kernel::class_ref, "print", &print);
		static_method<Arg::Count, Arg::Values>(Kernel::class_ref, "puts", &puts);
		static_method<Arg::Self, Arg::Value>(Kernel::class_ref, "load", &load);
		static_method<Arg::Self, Arg::Value>(Kernel::class_ref, "require", &load);
		static_method<Arg::Self, Arg::Value>(Kernel::class_ref, "require_relative", &load);
		static_method<Arg::Count, Arg::Values>(Kernel::class_ref, "raise", &raise);
	}
};
