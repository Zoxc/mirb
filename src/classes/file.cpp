#include "file.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "array.hpp"
#include "fixnum.hpp"
#include "../number.hpp"
#include "../recursion-detector.hpp"

namespace Mirb
{
	CharArray File::normalize_path(const CharArray &path)
	{
		CharArray result(path);

		if(!result.size())
			return result;

		result.localize();

#ifdef WIN32
		for(size_t i = 0; i < result.size(); ++i)
			if(result[i] == '\\')
				result[i] = '/';
#endif

		if(result[result.size()-1] == '/')
			result.shrink(result.size() - 1);

		return result;
	}
	
	bool File::absolute_path(const CharArray &path)
	{
#ifdef WIN32
		return path.size() > 1 && path[1] == ':';
#else
		return path.size() > 0 && path[0] == '/';
#endif
	}

	void JoinSegments::push(const CharArray &path)
	{
		File::normalize_path(path).split([&](const CharArray &part) {
			if(part.size() || (segments.size() == 0))
				segments.push_back(part);
		}, CharArray("/"));
	}

	CharArray JoinSegments::join()
	{
		int pos = 0;

		CharArray result;

		for(size_t i = segments.size(); i-- > 0;)
		{
			if(segments[i] == ".")
				continue;
				
			if(segments[i] == "..")
			{
				pos--;
				continue;
			}

			if(pos < 0)
				pos++;
			else
				result = segments[i] + "/" + result;
		}

		if(result.size())
			result.shrink(result.size() - 1);

		return result;
	}
	
	CharArray File::expand_path(CharArray relative)
	{
		if(!absolute_path(relative))
			return join(thread_context->current_directory, relative);
		else
			return relative;
	};
	
	CharArray File::expand_path(CharArray relative, CharArray from)
	{
		if(!absolute_path(relative))
		{
			if(!absolute_path(from))
				from = expand_path(from);

			return join(from, relative);
		}
		else
			return relative;
	};
	
	bool File::fnmatch(const CharArray &path, const CharArray &pattern)
	{
		size_t i = 0;
		size_t p = 0;
		size_t pr = pattern.size();

		for(size_t j = 0; j < pattern.size(); ++j)
			if(pattern[j] == '*')
				pr -= 1;

		if(pr > path.size())
			return false;

		while(p < pattern.size())
		{
			switch(pattern[p])
			{
				case '*':
				{
					if(++p >= pattern.size())
						return true;

					char_t terminator = pattern[p];

					if(terminator == '*')
						break;

					if(terminator == '?')
					{
						while(pr < path.size() - i)
							i++;

						break;
					}

					if(i >= path.size())
						return true;

					while(path[i] != terminator)
					{
						i++;

						if(i >= path.size())
							return false;
					}

					break;
				}

				case '?':
					if(i >= path.size())
						return false;

					p++, i++;
					break;

				default:
				{
					if(i >= path.size())
						return false;

					if(pattern[p] != path[i])
						return false;

					p++, i++;

					break;
				}
			}
		}

		return i == path.size() && p == pattern.size();
	}
	
	value_t File::rb_expand_path(String *relative, String *absolute)
	{
		if(absolute)
			return File::expand_path(relative->string, absolute->string).to_string();
		else
			return File::expand_path(relative->string).to_string();
	}
	
	CharArray File::join(const CharArray &left, const CharArray &right)
	{
		JoinSegments joiner;

		joiner.push(left);
		joiner.push(right);
		
		return joiner.join();
	}

	CharArray File::basename(CharArray path)
	{
		CharArray result = normalize_path(path);

		size_t i = result.size();

		while(i-- > 0)
			if(const_cast<const CharArray &>(result)[i] == '/')
				break;
		
		return result.copy(++i, result.size());
	}
	
	CharArray File::dirname(CharArray path)
	{
		CharArray result = File::normalize_path(path);

		for(size_t i = result.size(); i-- > 0;)
			if(result[i] == '/')
			{
				if(i)
				{
					result.shrink(i);
					return result;
				}
				else
					return CharArray("/");
			}
		
		return CharArray(".");
	}
	
	value_t rb_dirname(String *path)
	{
		return File::dirname(path->string).to_string();
	}
	
	void do_join(JoinSegments &joiner, size_t argc, value_t argv[])
	{
		for(size_t i = 0; i < argc; ++i)
		{
			auto array = try_cast<Array>(argv[i]);

			if(array)
			{
				RecursionDetector<RecursionType::File_join> rd(array);

				do_join(joiner, array->size(), array->vector.raw());
				continue;
			}

			joiner.push(raise_cast<String>(argv[i])->string);
		}
	}
	
	value_t rb_join(size_t argc, value_t argv[])
	{
		JoinSegments joiner;

		do_join(joiner, argc, argv);

		return joiner.join().to_string();
	}
	
	value_t exists(String *path)
	{
		return Value::from_bool(Platform::file_exists(path->string));
	}

	value_t file(String *path)
	{
		return Value::from_bool(Platform::is_file(path->string));
	}

	value_t rb_size(String *path)
	{
		return Value::from_bool(Platform::has_size(path->string));
	}

	value_t rb_delete(size_t argc, value_t argv[])
	{
		Platform::wrap([&] {
			for(size_t i = 0; i < argc; ++i)
			{
				auto str = raise_cast<String>(argv[i]);

				Platform::remove_file(str->string);
			}
		});

		return Number(argc).to_value();
	}

	value_t directory(String *path)
	{
		return Value::from_bool(Platform::is_directory(path->string));
	}

	value_t executable(String *path)
	{
		return Value::from_bool(Platform::is_executable(path->string));
	}
	
	value_t rb_fnmatch(String *pattern, String *path)
	{
		return Value::from_bool(File::fnmatch(path->string, pattern->string));
	}
	
	IO *File::open(String *path, String *mode)
	{
		Stream *file;
		
		size_t access = 0;
		Platform::Mode create_mode;

		if(!mode)
		{
			access = Platform::Read;
			create_mode = Platform::Open;
		}
		else
		{
			size_t len = 0;

			if(!mode->string.size())
				raise(context->argument_error, "Invalid mode string");

			auto set_mode = [&](size_t new_access, Platform::Mode new_mode, size_t alt_access)
			{
				len++;

				access |= new_access;
				create_mode = new_mode;

				if(len < mode->string.size() && mode->string[len] == '+')
				{
					len++;
					access |= alt_access;
				}
			};

			while(len < mode->string.size())
			{
				switch(mode->string[len])
				{
					case 'r':
						set_mode(Platform::Read, Platform::Open, Platform::Write);
						break;
				
					case 'w':
						set_mode(Platform::Write, Platform::CreateTruncate, Platform::Read);
						break;
			
					case 'a':
						set_mode(Platform::Write, Platform::CreateAppend, Platform::Read);
						break;
						
					case 'b':
					case 't':
						len++;
						break;

					default:
						raise(context->argument_error, "Unknown mode '" + CharArray(mode->string[len]) + "'");
				}
			}
		}

		Platform::wrap([&] {
			file = Platform::open(path->string, access, create_mode);
		});

		auto result = new (collector) IO(file, context->file_class);

		set_var(result, "@path", String::dup(path));

		return result;
	}
	
	value_t rb_open(String *path, String *mode, value_t block)
	{
		auto result = File::open(path, mode);

		if(block)
		{
			OnStack<1> os(result);

			Finally finally([&]{
				result->close();
			});

			return yield(block, result);
		}
		else
			return result;
	}
	
	value_t rb_new(String *path, String *mode)
	{
		return File::open(path, mode);
	}
	
	value_t rb_path(value_t file)
	{
		return get_var(file, "@path");
	}
	
	value_t path(String *str)
	{
		return str;
	}
	
	void File::initialize()
	{
		context->file_class = define_class("File", context->io_class);
		
		method<Self<Value>, &rb_path>(context->file_class, "path");

		singleton_method<String, Optional<String>, &rb_new>(context->file_class, "new");
		singleton_method<String, Optional<String>, Arg::Block, &rb_open>(context->file_class, "open");

		singleton_method<String, &exists>(context->file_class, "exists?");
		singleton_method<String, &exists>(context->file_class, "exist?");
		singleton_method<String, &path>(context->file_class, "path");
		singleton_method<String, &rb_size>(context->file_class, "size?");
		singleton_method<Arg::Count, Arg::Values, &rb_delete>(context->file_class, "delete");
		singleton_method<String, &file>(context->file_class, "file?");
		singleton_method<String, &directory>(context->file_class, "directory?");
		singleton_method<String, &executable>(context->file_class, "executable?");
		singleton_method<String, String, &rb_fnmatch>(context->file_class, "fnmatch?");
		singleton_method<String, String, &rb_fnmatch>(context->file_class, "fnmatch");

		singleton_method<String, Optional<String>, &rb_expand_path>(context->file_class, "expand_path");
		singleton_method<String, &rb_dirname>(context->file_class, "dirname");
		singleton_method<Arg::Count, Arg::Values, &rb_join>(context->file_class, "join");

		set_const(context->file_class, Symbol::get("SEPARATOR"), String::get("/"));
	}
};

