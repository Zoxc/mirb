#include "regexp.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t quote(String *str)
	{
		return str;
	}
	
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);

		singleton_method<Arg::Class<String>>(context->regexp_class, "quote", &quote);
	}
};

