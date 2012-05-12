#include "regexp.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

#ifdef WIN32
	#define PCRE_STATIC 1
#endif

#include <pcre.h>

namespace Mirb
{
	value_t quote(String *str)
	{
		return str;
	}

	Regexp *Regexp::allocate(const CharArray &pattern)
	{
		CharArray pattern_cstr = pattern.c_str();
		const char *error;
		int err_offset;

		pcre *re = pcre_compile(pattern_cstr.c_str_ref(), 0, &error, &err_offset, 0);

		if(!re)
		{
			raise(context->syntax_error, "Error in regular expression: " + CharArray((const char_t *)error));
			return 0;
		}

		Regexp *result = Collector::allocate<Regexp>(context->regexp_class);

		result->re = re;
		result->pattern = pattern;

		return result;
	}
	
	value_t Regexp::to_s(Regexp *obj)
	{
		return ("/" + obj->pattern  + "/").to_string();
	}
	
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);

		method<Arg::SelfClass<Regexp>>(context->regexp_class, "to_s", &to_s);

		singleton_method<Arg::Class<String>>(context->regexp_class, "quote", &quote);
	}
};

