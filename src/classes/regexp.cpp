#include "regexp.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "array.hpp"
#include "../runtime.hpp"

#ifdef WIN32
	#define PCRE_STATIC 1
#endif

#include <pcre.h>

namespace Mirb
{
	Regexp::~Regexp()
	{
		if(re)
			pcre_free(re);
	}

	value_t quote(String *str)
	{
		return str;
	}
	
	bool Regexp::compile_pattern()
	{
		if(re)
			return true;

		CharArray pattern_cstr = pattern.c_str();
		const char *error;
		int err_offset;

		re = pcre_compile(pattern_cstr.c_str_ref(), 0, &error, &err_offset, 0);

		if(!re)
		{
			raise(context->syntax_error, "Error in regular expression: " + CharArray((const char_t *)error));
			return false;
		}

		return true;

	}

	Regexp *Regexp::allocate(const CharArray &pattern)
	{
		Regexp *result = Collector::allocate<Regexp>(context->regexp_class);

		result->pattern = pattern;

		if(!result->compile_pattern())
			return 0;

		return result;
	}

	value_t Regexp::rb_initialize(Regexp *obj, value_t pattern)
	{
		if(Value::of_type<Regexp>(pattern))
			obj->pattern = cast<Regexp>(pattern)->pattern;
		else if(Value::of_type<String>(pattern))
			obj->pattern = cast<String>(pattern)->string;
		else
			raise(context->runtime_error, "Expected a valid pattern");

		if(!obj->compile_pattern())
			return 0;

		return value_nil;
	}
	
	value_t Regexp::rb_allocate(Class *instance_of)
	{
		return Collector::allocate<Regexp>(instance_of);
	}
	
	value_t Regexp::to_s(Regexp *obj)
	{
		return ("/" + obj->pattern  + "/").to_string();
	}

	value_t Regexp::match(Regexp *obj, String *string)
	{
		if(!obj->compile_pattern())
			return 0;

		static const size_t vector_size = 16 * 3;

		int ovector[vector_size];
		
		int result = pcre_exec(obj->re, 0, string->string.c_str_ref(), string->string.size(), 0, PCRE_NEWLINE_ANYCRLF, ovector, vector_size);

		value_t match_data;

		if(result < 0)
			match_data = value_nil;
		else
		{
			auto data = Collector::allocate<Array>();

			for (int i = 0; i < result; i++) {
				data->vector.push(string->string.copy(ovector[2 * i], ovector[2 * i + 1] - ovector[2 * i]).to_string());
			}

			match_data = data;
		}

		return match_data;
	}
	
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);
		
		method<Arg::Self<Arg::Class<Regexp>>>(context->regexp_class, "to_s", &to_s);
		method<Arg::Self<Arg::Class<Regexp>>, Arg::Value>(context->regexp_class, "initialize", &rb_initialize);
		method<Arg::Self<Arg::Class<Regexp>>, Arg::Class<String>>(context->regexp_class, "match", &match);

		singleton_method<Arg::Class<String>>(context->regexp_class, "quote", &quote);
		singleton_method<Arg::Self<Arg::Class<Class>>>(context->regexp_class, "allocate", &rb_allocate);
	}
};

