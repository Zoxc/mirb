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
	
	void Regexp::compile_pattern()
	{
		if(re)
			return;

		CharArray pattern_cstr = pattern.c_str();
		const char *error;
		int err_offset;

		re = pcre_compile(pattern_cstr.c_str_ref(), 0, &error, &err_offset, 0);

		if(!re)
			raise(context->syntax_error, "Error in regular expression: " + CharArray((const char_t *)error));
	}

	Regexp *Regexp::allocate(const CharArray &pattern)
	{
		Regexp *result = Collector::allocate<Regexp>(context->regexp_class);

		result->pattern = pattern;
		result->compile_pattern();

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

		obj->compile_pattern();

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
	
	const size_t Regexp::vector_size = 16 * 3;

	value_t Regexp::match(Regexp *obj, String *string)
	{
		obj->compile_pattern();

		int ovector[vector_size];
		
		int result = pcre_exec(obj->re, 0, string->string.c_str_ref(), string->string.size(), 0, PCRE_NEWLINE_ANYCRLF, ovector, vector_size);

		value_t match_data;

		if(result <= 0)
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
	
	CharArray Regexp::gsub(const CharArray &input, Regexp *pattern, const CharArray &replacement, bool &changed)
	{
		pattern->compile_pattern();
		
		int ovector[vector_size];

		CharArray result;
		int prev = 0;
		
		auto push = [&](int to) {
			result += input.copy(prev, to - prev);
		};

		changed = false;
		
		while(true)
		{
			int groups = pcre_exec(pattern->re, 0, input.c_str_ref(), input.size(), prev, PCRE_NEWLINE_ANYCRLF, ovector, vector_size);

			if(groups <= 0)
			{
				push(input.size());
				return result;
			}
			else
			{
				changed = true;

				int start = ovector[0];
				int stop = ovector[1];

				for (int i = 0; i < groups; i++) {
					start = std::min(start, ovector[2 * i]);
					stop = std::max(stop, ovector[2 * i + 1]);
				}

				push(start);
				prev = stop;
				result += replacement;
			}
		}
	}
	
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);
		
		method<Arg::Self<Arg::Class<Regexp>>, &to_s>(context->regexp_class, "to_s");
		method<Arg::Self<Arg::Class<Regexp>>, Arg::Value, &rb_initialize>(context->regexp_class, "initialize");
		method<Arg::Self<Arg::Class<Regexp>>, Arg::Class<String>, &match>(context->regexp_class, "match");

		singleton_method<Arg::Class<String>, &quote>(context->regexp_class, "quote");
		singleton_method<Arg::Self<Arg::Class<Class>>, &rb_allocate>(context->regexp_class, "allocate");
	}
};

