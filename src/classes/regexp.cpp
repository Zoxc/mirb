#include "regexp.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "array.hpp"
#include "../runtime.hpp"
#include <climits>

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
	
	value_t Regexp::source(Regexp *obj)
	{
		return obj->pattern.to_string();
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
	
	int Regexp::match(const CharArray &input, int *ovector, size_t offset)
	{
		mirb_runtime_assert(offset <= INT_MAX);
		return pcre_exec(re, 0, input.c_str_ref(), input.size(), (int)offset, PCRE_NEWLINE_ANYCRLF, ovector, vector_size);
	}

	value_t Regexp::rb_match(Regexp *obj, String *string)
	{
		obj->compile_pattern();

		int ovector[vector_size];
		
		int result = obj->match(string->string, ovector, 0);

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
		CharArray result;
		
		changed = false;

		pattern->split(input, [&](const CharArray &data) { result += data; }, [&](size_t start, size_t stop) { changed = true; result += replacement; });

		return result;
	}
	
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);
		
		method<Arg::Self<Arg::Class<Regexp>>, &source>(context->regexp_class, "source");
		method<Arg::Self<Arg::Class<Regexp>>, &to_s>(context->regexp_class, "to_s");
		method<Arg::Self<Arg::Class<Regexp>>, Arg::Value, &rb_initialize>(context->regexp_class, "initialize");
		method<Arg::Self<Arg::Class<Regexp>>, Arg::Class<String>, &rb_match>(context->regexp_class, "match");

		singleton_method<Arg::Class<String>, &quote>(context->regexp_class, "quote");
		singleton_method<Arg::Self<Arg::Class<Class>>, &rb_allocate>(context->regexp_class, "allocate");
	}
};

