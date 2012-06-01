#include "dir.hpp"
#include "file.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "array.hpp"

namespace Mirb
{
	value_t pwd()
	{
		return thread_context->current_directory.to_string();
	}
	
	value_t chdir(String *path, value_t block)
	{
		CharArray old = thread_context->current_directory;

		if(!Platform::is_directory(path->string))
			raise(context->system_call_error, "'"+ path->string + "' in not a directory");

		thread_context->current_directory = path->string;
		
		OnStackString<1> os(old);

		Finally finally([&] {
			thread_context->current_directory = old;
		});

		return yield(block);
	}
	
	value_t mkdir(String *path)
	{
		Platform::wrap([&] {
			Platform::mkdir(path->string);
		});

		return Fixnum::from_int(0);
	}
	
	value_t rb_delete(String *path)
	{
		Platform::wrap([&] {
			Platform::remove_dir(path->string);
		});

		return Fixnum::from_int(0);
	}
	
	value_t entries(String *path)
	{
		auto array = new (collector) Array;
		
		Platform::wrap([&] {
			Platform::list_dir(path->string, false, [&](const CharArray &filename, bool) {
				array->vector.push(filename.to_string());
			});
		});

		return array;
	}

	bool is_pattern(const CharArray &fn)
	{
		for(size_t i = 0; i < fn.size(); ++i)
		{
			switch(fn[i])
			{
				case '*':
				case '?':
					return true;
				default:
					break;
			}
		}

		return false;
	}
	
	void glob(Array *array, std::vector<CharArray> &segments, size_t i, CharArray path)
	{
		auto join_path = [&](CharArray filename) -> CharArray {
			if(path.size())
				return path + "/" + filename;
			else
				return filename;
		};

		while((i < segments.size() - 1) && !is_pattern(segments[i]))
		{
			path = join_path(segments[i]);
			i++;
		}

		if(segments[i] == "**" && (i + 1 < segments.size()))
		{
			Platform::list_dir(File::expand_path(path), true, [&](const CharArray &filename, bool directory) {
				if(directory)
					glob(array, segments, i, join_path(filename));
			});
			
			glob(array, segments, i + 1, path);
		}
		else if(i + 1 < segments.size())
		{
			Platform::list_dir(File::expand_path(path), true, [&](const CharArray &filename, bool directory) {
				if(directory && File::fnmatch(filename, segments[i]))
					glob(array, segments, i + 1, join_path(filename));
			});
		}
		else
		{
			Platform::list_dir(File::expand_path(path), true, [&](const CharArray &filename, bool) {
				if(File::fnmatch(filename, segments[i]))
					array->vector.push(String::get(join_path(filename)));
			});
		}
	}
	
	value_t rb_glob(String *pattern)
	{
		auto array = new (collector) Array;
		
		Platform::wrap([&] {
			std::vector<CharArray> segments;

			CharArray path = File::normalize_path(pattern->string);

			path.split([&](const CharArray &part) {
				segments.push_back(part);
			}, CharArray("/"));

			if(File::absolute_path(path))
				glob(array, segments, 1, segments[0].size() ? segments[0] : "/");
			else
				glob(array, segments, 0, "");
		});

		return array;
	}
	
	void Dir::initialize()
	{
		context->dir_class = define_class("Dir", context->object_class);
		
		singleton_method<String, &mkdir>(context->dir_class, "mkdir");
		singleton_method<String, &rb_delete>(context->dir_class, "delete");
		singleton_method<String, Arg::Block, &chdir>(context->dir_class, "chdir");
		singleton_method<String, &entries>(context->dir_class, "entries");
		singleton_method<String, &rb_glob>(context->dir_class, "glob");
		singleton_method<String, &rb_glob>(context->dir_class, "[]");

		singleton_method<&pwd>(context->dir_class, "pwd");
	}
};

