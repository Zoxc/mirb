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
		return File::normalize_path(Platform::cwd()).to_string();
	}
	
	value_t entries(String *path)
	{
		auto array = new (collector) Array;

		if(!Platform::list_dir(path->string, [&](const CharArray &filename) {
			array->vector.push(filename.to_string());
		}))
			return raise(context->system_call_error, "Unable to list directory '" + path->string + "'");

		return array;
	}
	
	value_t glob(String *pattern)
	{
		auto array = new (collector) Array;

		CharArray cwd = Platform::cwd();

		if(!Platform::list_dir(cwd, [&](const CharArray &filename) {
			if(File::fnmatch(filename, pattern->string))
				array->vector.push(filename.to_string());
		}))
			return raise(context->system_call_error, "Unable to list directory '" + cwd + "'");

		return array;
	}
	
	void Dir::initialize()
	{
		context->dir_class = define_class("Dir", context->object_class);
		
		singleton_method<Arg::Class<String>>(context->dir_class, "entries", &entries);
		singleton_method<Arg::Class<String>>(context->dir_class, "glob", &glob);
		singleton_method<Arg::Class<String>>(context->dir_class, "[]", &glob);

		singleton_method<>(context->dir_class, "pwd", &pwd);
	}
};

