#include "regexp.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "array.hpp"
#include "fixnum.hpp"
#include "../runtime.hpp"
#include "../internal.hpp"
#include "../number.hpp"
#include <climits>

#ifdef WIN32
	#define PCRE_STATIC 1
#endif

#include <pcre.h>

namespace Mirb
{
	Regexp::Regexp(const Regexp &other) : Object(other), re(nullptr), pattern(other.pattern)
	{
	}

	Regexp::~Regexp()
	{
		if(re)
			pcre_free(re);
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
		Regexp *result = new (collector) Regexp(context->regexp_class);

		result->pattern = pattern;
		result->compile_pattern();

		return result;
	}
	
	value_t Regexp::source(Regexp *obj)
	{
		return obj->pattern.to_string();
	}

	value_t Regexp::escape(String *str)
	{
		CharArray result;

		str->string.each_char([&](char_code_t c, size_t) {
			if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ')
				result += CharArray(c);
			else
			{
				if(c < 32 || c >= 127)
					result +=  "\\x" + Number((intptr_t)c).to_string(16).rjust(2, "0");
				else
					result +=  "\\" + CharArray(c);
			}
		});
		
		return result.to_string();
	}

	value_t Regexp::rb_initialize(Regexp *obj, value_t pattern)
	{
		if(of_type<Regexp>(pattern))
			obj->pattern = cast<Regexp>(pattern)->pattern;
		else if(of_type<String>(pattern))
			obj->pattern = cast<String>(pattern)->string;
		else
			raise(context->runtime_error, "Expected a valid pattern");

		obj->compile_pattern();

		return value_nil;
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
			auto data = Array::allocate();

			for (int i = 0; i < result; i++) {
				data->vector.push(string->string.copy(ovector[2 * i], ovector[2 * i + 1] - ovector[2 * i]).to_string());
			}

			match_data = data;
		}

		return match_data;
	}
	
	value_t Regexp::case_equal(Regexp *self, value_t other)
	{
		auto str = try_cast<String>(other);

		if(!str)
			return value_false;
		
		int ovector[Regexp::vector_size];
		
		int result = self->match(str->string, ovector, 0);

		return Value::from_bool(result > 0);
	}

	value_t Regexp::rb_pattern(Regexp *self, String *str)
	{
		int ovector[Regexp::vector_size];
		
		int result = self->match(str->string, ovector, 0);

		if(result <= 0)
			return value_nil;

		return Fixnum::from_int(ovector[0]);
	}

	CharArray Regexp::gsub(const CharArray &input, Regexp *pattern, const CharArray &replacement, bool &changed)
	{
		CharArray result;
		
		changed = false;

		pattern->split(input, [&](const CharArray &data) { result += data; }, [&](size_t, size_t) { changed = true; result += replacement; });

		return result;
	}
	
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);
		
		internal_allocator<Regexp, &Context::regexp_class>();
		
		method<Self<Regexp>, &source>(context->regexp_class, "source");
		method<Self<Regexp>, &to_s>(context->regexp_class, "to_s");
		method<Self<Regexp>, Value, &rb_initialize>(context->regexp_class, "initialize");
		method<Self<Regexp>, String, &rb_match>(context->regexp_class, "match");
		method<Self<Regexp>, String, &rb_pattern>(context->regexp_class, "=~");
		method<Self<Regexp>, Value, &case_equal>(context->regexp_class, "===");
		
		singleton_method<String, &escape>(context->regexp_class, "escape");
		singleton_method<String, &escape>(context->regexp_class, "quote");
	}
};

