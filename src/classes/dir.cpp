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

		Platform::list_dir(path->string, [&](const CharArray &filename) {
			array->vector.push(filename.to_string());
		});

		return array;
	}
	
	void Dir::initialize()
	{
		context->dir_class = define_class("Dir", context->object_class);
		
		singleton_method<Arg::Class<String>>(context->dir_class, "entries", &entries);

		singleton_method<>(context->dir_class, "pwd", &pwd);
	}
};

