#include "string.hpp"
#include "symbol.hpp"
#include "array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t String::from_symbol(Symbol *symbol)
	{
		return Collector::allocate<String>(symbol->string);
	}
	
	value_t String::get(const CharArray &string)
	{
		return Collector::allocate<String>(string);
	}
			
	value_t String::from_string(const char *c_str)
	{
		return Collector::allocate<String>((const char_t *)c_str, std::strlen(c_str));
	}
	
	value_t String::inspect(String *self)
	{
		CharArray result = "\"" + self->string + "\"";

		return result.to_string();
	}
	
	value_t String::concat(String *self, String *other)
	{
		self->string.append(other->string);

		return self;
	}
	
	value_t split(String *self, String *sep)
	{
		auto result = Collector::allocate<Array>();

		self->string.split([&](const CharArray &match) {
			result->vector.push(match.to_string());
		}, sep->string);

		return result;
	}
	
	value_t String::to_s(value_t self)
	{
		return self;
	}
	
	value_t match(value_t self, value_t regexp)
	{
		if(!Value::of_type<Regexp>(regexp))
		{
			regexp = call(context->regexp_class, "new", value_nil, 1, &regexp);

			if(!regexp || type_error(regexp, context->regexp_class))
				return 0;
		}

		return call(regexp, "match", value_nil, 1, &self);
	}

	value_t String::equal(String *self, String *other)
	{
		return auto_cast(self->string == other->string);
	}
	
	value_t String::downcase(String *self)
	{
		CharArray result(self->string);

		result.downcase();

		return result.to_string();
	}

	value_t String::downcase_self(String *self)
	{
		if(self->string.downcase())
			return self;
		else
			return value_nil;
	}
	
	value_t String::upcase(String *self)
	{
		CharArray result(self->string);

		result.upcase();

		return result.to_string();
	}

	value_t String::upcase_self(String *self)
	{
		if(self->string.upcase())
			return self;
		else
			return value_nil;
	}

	void String::initialize()
	{
		method<Arg::Self, Arg::Value>(context->string_class, "match", &match);
		method<Arg::Self>(context->string_class, "to_s", &to_s);
		
		method<Arg::SelfClass<String>>(context->string_class, "downcase", &downcase);
		method<Arg::SelfClass<String>>(context->string_class, "downcase!", &downcase_self);
		method<Arg::SelfClass<String>>(context->string_class, "upcase", &upcase);
		method<Arg::SelfClass<String>>(context->string_class, "upcase!", &upcase_self);

		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "split", &split);
		method<Arg::SelfClass<String>>(context->string_class, "inspect", &inspect);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "==", &equal);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "concat", &concat);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "<<", &concat);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "+", &concat);
	}
};

