#include "file.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "fixnum.hpp"

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
		{
			try
			{
				return join(Platform::cwd(), relative);
			}
			catch(Exception *)
			{
				return CharArray();
			}
		}
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

	value_t dirname(String *path)
	{
		CharArray result = File::normalize_path(path->string);

		for(size_t i = result.size(); i-- > 0;)
			if(result[i] == '/')
			{
				if(i)
				{
					result.shrink(i);
					return result.to_string();
				}
				else
					return CharArray("/").to_string();
			}
		
		return CharArray(".").to_string();
	}
	
	value_t rb_join(size_t argc, value_t argv[])
	{
		JoinSegments joiner;

		for(size_t i = 0; i < argc; ++i)
		{
			if(type_error(argv[i], context->string_class))
				return 0;

			joiner.push(cast<String>(argv[i])->string);
		}

		return joiner.join().to_string();
	}
	
	value_t exists(String *path)
	{
		return auto_cast(Platform::file_exists(path->string));
	}
	
	value_t file(String *path)
	{
		return auto_cast(Platform::is_file(path->string));
	}
	
	value_t directory(String *path)
	{
		return auto_cast(Platform::is_directory(path->string));
	}
	
	value_t rb_fnmatch(String *pattern, String *path)
	{
		return auto_cast(File::fnmatch(path->string, pattern->string));
	}
	
	void File::initialize()
	{
		context->file_class = define_class("File", context->io_class);
		
		singleton_method<Arg::Class<String>>(context->file_class, "exists?", &exists);
		singleton_method<Arg::Class<String>>(context->file_class, "exist?", &exists);
		singleton_method<Arg::Class<String>>(context->file_class, "file?", &file);
		singleton_method<Arg::Class<String>>(context->file_class, "directory?", &directory);
		singleton_method<Arg::Class<String>, Arg::Class<String>>(context->file_class, "fnmatch?", &rb_fnmatch);
		singleton_method<Arg::Class<String>, Arg::Class<String>>(context->file_class, "fnmatch", &rb_fnmatch);

		singleton_method<Arg::Class<String>, Arg::DefaultClass<String>>(context->file_class, "expand_path", &rb_expand_path);
		singleton_method<Arg::Class<String>>(context->file_class, "dirname", &dirname);
		singleton_method<Arg::Count, Arg::Values>(context->file_class, "join", &rb_join);

		set_const(context->file_class, Symbol::get("SEPARATOR"), String::get("/"));
	}
};

