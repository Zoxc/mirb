#include "kernel.hpp"
#include "../classes/object.hpp"
#include "../classes/string.hpp"
#include "../classes/symbol.hpp"
#include "../classes/array.hpp"
#include "../classes/exception.hpp"
#include "../classes/exceptions.hpp"
#include "../classes/file.hpp"
#include "../platform/platform.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Kernel::at_exit(value_t block)
	{
		if(type_error(block, context->proc_class))
			return 0;

		context->at_exits.push(block);

		return block;
	}
	
	value_t Kernel::proc(value_t block)
	{
		return block;
	}
	
	value_t Kernel::block_given()
	{
		return Value::from_bool(context->frame->prev->block != value_nil);
	}
	
	value_t Kernel::benchmark(value_t block)
	{
		CharArray result;
		
		OnStackString<1> os(result);

		bool exception = false;

		result = Mirb::Platform::benchmark([&] {
			if(!yield(block))
				exception = true;
		}).format();

		return exception ? 0 : result.to_string();
	}
	
	value_t Kernel::eval(value_t obj, String *input)
	{
		CharArray c_str = input->string.c_str();
		CharArray filename("(eval)");

		return eval(obj, Symbol::get("in eval"), context->frame->prev->scope, c_str.str_ref(), c_str.str_length(), filename);
	}

	FILE *try_file(const CharArray &filename, CharArray &result)
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
			
			CharArray filename_rb = filename + ".rb";
			CharArray filename_rb_cstr = filename_rb.c_str();
			
			#ifndef WIN32
				if(stat(filename_rb_cstr.c_str_ref(), &buf) != -1)
					is_dir = S_ISDIR(buf.st_mode);
			#endif

			if(!is_dir)
				file = fopen(filename_rb_cstr.c_str_ref(), "rb");

			if(is_dir || !file)
				return 0;

			result = filename_rb;

			return file;
		}

		return file;
	}
	
	FILE *open_file(CharArray &filename, bool try_relative)
	{
		if(File::absolute_path(filename))
			return try_file(filename, filename);

		auto load_path = cast<Array>(get_global(Symbol::get("$:"))); // TODO: Turn $: into a Array field in Context
		FILE *result = nullptr;
		
		load_path->vector.each([&](value_t path) -> bool {
			auto str = cast<String>(path)->string;

			JoinSegments joiner;
			
			joiner.push(str);
			joiner.push(filename);

			str = joiner.join();

			result = try_file(str, filename);

			return result == nullptr;
		});

		if(result)
			return result;

		if(try_relative)
			return try_file(filename, filename);
		else
			return nullptr;
	}
	
	bool Kernel::read_file(CharArray filename, bool try_relative, bool require, CharArray &full_path, bool& loaded, char_t *&data, size_t &length)
	{
		loaded = false;

		FILE* file = open_file(filename, try_relative);

		if(!file)
		{
			raise(context->load_error, "Unable to find file '" + filename + "'");
			return false;
		}
		
		full_path = File::expand_path(filename);

		loaded = !context->loaded.each([&](value_t path) { return cast<String>(path)->string != full_path; });

		if(require && loaded)
		{
			fclose(file);
			return true;
		}

		if(!loaded)
			context->loaded.push(full_path.to_string());

		fseek(file, 0, SEEK_END);

		length = ftell(file);

		fseek(file, 0, SEEK_SET);

		data = (char_t *)malloc(length + 1);

		if(!data)
		{
			fclose(file);
			raise(context->load_error, "Unable to allocate memory for file content '" + filename + "' (" + CharArray::uint(length) + " bytes)");
			return false;
		}

		if(fread(data, 1, length, file) != length)
		{
			free(data);
			fclose(file);

			raise(context->load_error, "Unable to read content of file '" + filename + "'");
			return false;
		}

		data[length] = 0;

		fclose(file);

		return true;
	}
	
	value_t run_file(value_t self, CharArray filename, bool try_relative, bool require)
	{
		char_t *data;
		size_t length;
		bool loaded;
		CharArray full_path;

		if(!Kernel::read_file(filename, try_relative, require, full_path, loaded, data, length))
			return 0;

		if(require && loaded)
			return value_nil;

		value_t result = eval(self, Symbol::get("main"), context->object_scope, data, length, full_path, true);

		if(result)
		{
			// TODO: Remove loaded file from loaded list

			return value_nil;
		}
		else
			return 0;
	}
	
	value_t Kernel::load(String *filename)
	{
		return run_file(context->main, filename->string, true, false);
	}
	
	value_t Kernel::require(String *filename)
	{
		return run_file(context->main, filename->string, false, true);
	}
	
	value_t Kernel::require_relative(String *filename)
	{
		return run_file(context->main, filename->string, true, true);
	}
	
	value_t Kernel::print(size_t argc, value_t argv[])
	{
		for(size_t i = 0; i < argc; ++i)
		{
			value_t arg = argv[i];

			if(Value::type(arg) != Value::String)
				arg = call(arg, "to_s");

			String *str = raise_cast<String>(arg);

			if(!str)
				return 0;
			
			std::cout << str->string.get_string();
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
			
			if(!arg)
				return 0;
			
			String *str = raise_cast<String>(arg);

			if(!str)
				return 0;
			
			std::cout << str->string.get_string() << "\n";
		}
		
		return value_nil;
	}
	
	value_t Kernel::raise(value_t first, String *str)
	{
		Class *instance_of = context->runtime_error;
		String *message = 0;

		if(str)
		{
			instance_of = raise_cast<Class>(first);

			if(!instance_of)
				return 0;

			message = str;
		}
		else if(first)
		{
			if(Value::type(first) == Value::String)
				message = cast<String>(first);
			else if(Value::type(first) == Value::Class && subclass_of(context->exception_class, cast<Class>(first)))
				instance_of = cast<Class>(first);
			else
			{
				type_error(first, "Exception subclass or String");
				return 0;
			}
		}

		Exception *exception = Collector::allocate<Exception>(instance_of, message, Mirb::backtrace());

		return raise(exception);
	}

	value_t Kernel::backtrace()
	{
		return StackFrame::get_backtrace(Mirb::backtrace());
	}
	
	value_t cast_array(value_t value)
	{
		auto array = try_cast<Array>(value);

		if(array)
			return array;

		auto method = lookup_method(class_of(value), Symbol::get("to_a"));

		if(method)
			return raise_cast<Array>(call_argv(method->block, value, Symbol::get("to_a"), method->scope, value_nil, 0, 0));
		else
		{
			array = new (collector) Array;
			array->vector.push(value);
			return array;
		}
	}

	void Kernel::initialize()
	{
		context->kernel_module = define_module("Kernel");

		include_module(context->object_class, context->kernel_module);
		
		method<Arg::Value>(context->kernel_module, "Array", &cast_array);
		method(context->kernel_module, "block_given?", &block_given);
		method<Arg::Block>(context->kernel_module, "at_exit", &at_exit);
		method<Arg::Block>(context->kernel_module, "proc", &proc);
		method<Arg::Block>(context->kernel_module, "lambda", &proc);
		method<Arg::Block>(context->kernel_module, "benchmark", &benchmark);
		method(context->kernel_module, "backtrace", &backtrace);
		method<Arg::Self<Arg::Value>, Arg::Class<String>>(context->kernel_module, "eval", &eval);
		method<Arg::Count, Arg::Values>(context->kernel_module, "print", &print);
		method<Arg::Count, Arg::Values>(context->kernel_module, "puts", &puts);
		method<Arg::Class<String>>(context->kernel_module, "load", &load);
		method<Arg::Class<String>>(context->kernel_module, "require", &require);
		method<Arg::Class<String>>(context->kernel_module, "require_relative", &require_relative);
		method<Arg::Default<Arg::Value>, Arg::Default<Arg::Class<String>>>(context->kernel_module, "raise", &raise);
	}
};
