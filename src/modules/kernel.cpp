#include "kernel.hpp"
#include "../classes/object.hpp"
#include "../classes/string.hpp"
#include "../classes/symbol.hpp"
#include "../classes/array.hpp"
#include "../classes/exception.hpp"
#include "../classes/exceptions.hpp"
#include "../classes/file.hpp"
#include "../classes/fixnum.hpp"
#include "../platform/platform.hpp"
#include "../char-array.hpp"
#include "../global.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Kernel::at_exit(value_t block)
	{
		get_proc(block);

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

		auto load_path = cast<Array>(context->load_paths);
		FILE *result = nullptr;
		
		load_path->vector.each([&](value_t path) -> bool {
			auto str = try_cast<String>(path);
			
			if(!str)
				return true;
			
			JoinSegments joiner;
			
			joiner.push(str->string);
			joiner.push(filename);

			result = try_file(joiner.join(), filename);

			return result == nullptr;
		});

		if(result)
			return result;

		if(try_relative)
			return try_file(filename, filename);
		else
			return nullptr;
	}
	
	void Kernel::read_file(CharArray filename, bool try_relative, bool require, CharArray &full_path, bool& loaded, char_t *&data, size_t &length)
	{
		loaded = false;

		FILE* file = open_file(filename, try_relative);

		if(!file)
			raise(context->load_error, "Unable to find file '" + filename + "'");
		
		full_path = File::expand_path(filename);

		loaded = !context->loaded_files->vector.each([&](value_t path) { return cast<String>(path)->string != full_path; });

		if(require && loaded)
		{
			fclose(file);
			return;
		}

		if(!loaded)
			context->loaded_files->vector.push(full_path.to_string());

		fseek(file, 0, SEEK_END);

		length = ftell(file);

		fseek(file, 0, SEEK_SET);

		data = (char_t *)malloc(length + 1);

		if(!data)
		{
			fclose(file);
			raise(context->load_error, "Unable to allocate memory for file content '" + filename + "' (" + CharArray::uint(length) + " bytes)");
		}

		if(fread(data, 1, length, file) != length)
		{
			free(data);
			fclose(file);

			raise(context->load_error, "Unable to read content of file '" + filename + "'");
		}

		data[length] = 0;

		fclose(file);
	}
	
	value_t run_file(value_t self, CharArray filename, bool try_relative, bool require)
	{
		char_t *data;
		size_t length;
		bool loaded;
		CharArray full_path;
		
		OnStackString<1> oss(full_path);

		try
		{
			Kernel::read_file(filename, try_relative, require, full_path, loaded, data, length);

			if(require && loaded)
				return value_nil;

			return eval(self, Symbol::get("main"), context->object_scope, data, length, full_path, true);
		}
		catch(InternalException e)
		{
			OnStack<1> os(e.value);

			Array::rb_delete(context->loaded_files, full_path.to_string());

			throw e;
		}
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
			context->console_output->print(cast_string(argv[i])->string);
		
		return value_nil;
	}
	
	value_t Kernel::puts(size_t argc, value_t argv[])
	{
		for(size_t i = 0; i < argc; ++i)
			context->console_output->puts(cast_string(argv[i])->string);
		
		return value_nil;
	}
	
	value_t Kernel::raise(value_t first, String *str)
	{
		Class *instance_of = context->runtime_error;
		String *message = 0;

		if(str)
		{
			instance_of = raise_cast<Class>(first);

			message = str;
		}
		else if(first)
		{
			if(Value::type(first) == Value::String)
				message = cast<String>(first);
			else if(Value::type(first) == Value::Class && subclass_of(context->exception_class, cast<Class>(first)))
				instance_of = cast<Class>(first);
			else
				type_error(first, "Exception subclass or String");
		}

		Exception *exception = Collector::allocate<Exception>(instance_of, message, Mirb::backtrace());

		throw InternalException(exception);
	}
	
	value_t Kernel::exit(intptr_t result)
	{
		throw InternalException(Collector::allocate<SystemExit>(result == Fixnum::undef ? 0 : result));
	}

	value_t Kernel::backtrace()
	{
		return StackFrame::get_plain_backtrace(Mirb::backtrace());
	}
	
	value_t Kernel::rb_cast_array(value_t value)
	{
		return cast_array(value);
	}
	
	value_t Kernel::rb_cast_string(value_t value)
	{
		return cast_string(value);
	}

	value_t Kernel::rb_cast_integer(value_t value)
	{
		return cast_integer(value);
	}

	void Kernel::initialize()
	{
		context->kernel_module = define_module("Kernel");

		include_module(context->object_class, context->kernel_module);
		
		method<Arg::Value, &Kernel::rb_cast_array>(context->kernel_module, "Array");
		method<Arg::Value, &Kernel::rb_cast_string>(context->kernel_module, "String");
		method<Arg::Value, &Kernel::rb_cast_integer>(context->kernel_module, "Integer");
		method<&block_given>(context->kernel_module, "block_given?");
		method<Arg::Block, &at_exit>(context->kernel_module, "at_exit");
		method<Arg::Block, &proc>(context->kernel_module, "proc");
		method<Arg::Block, &proc>(context->kernel_module, "lambda");
		method<Arg::Block, &benchmark>(context->kernel_module, "benchmark");
		method<&backtrace>(context->kernel_module, "backtrace");
		method<Arg::Self<Arg::Value>, Arg::Class<String>, &eval>(context->kernel_module, "eval");
		method<Arg::Optional<Arg::Fixnum>, &exit>(context->kernel_module, "exit");
		method<Arg::Count, Arg::Values, &print>(context->kernel_module, "print");
		method<Arg::Count, Arg::Values, &puts>(context->kernel_module, "puts");
		method<Arg::Class<String>, &load>(context->kernel_module, "load");
		method<Arg::Class<String>, &require>(context->kernel_module, "require");
		method<Arg::Class<String>, &require_relative>(context->kernel_module, "require_relative");
		method<Arg::Optional<Arg::Value>, Arg::Optional<Arg::Class<String>>, &raise>(context->kernel_module, "raise");
	}
};
