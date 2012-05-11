#include "dir.hpp"
#include "file.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "fixnum.hpp"

namespace Mirb
{
	value_t pwd()
	{
		return normalize_path(Platform::cwd()).to_string();
	}
	
	void Dir::initialize()
	{
		context->dir_class = define_class("Dir", context->object_class);
		
		singleton_method<>(context->dir_class, "pwd", &pwd);
	}
};

