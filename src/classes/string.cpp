#include "string.hpp"
#include "symbol.hpp"
#include "array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t String::from_symbol(Symbol *symbol)
	{
		return auto_cast(Collector::allocate<String>(symbol->string));
	}
	
	value_t String::from_string(const char *c_str)
	{
		return auto_cast(Collector::allocate<String>((const char_t *)c_str, std::strlen(c_str)));
	}
	
	value_t String::inspect(value_t obj)
	{
		auto self = cast<String>(obj);

		CharArray result = "\"" + self->string + "\"";

		return result.to_string();
	}
	
	value_t String::to_s(value_t obj)
	{
		return obj;
	}
	
	value_t String::concat(value_t obj, value_t other)
	{
		auto self = cast<String>(obj);
		auto other_str = cast<String>(other);

		self->string.append(other_str->string);

		return obj;
	}
	
	value_t split(String *self, String *sep)
	{
		auto result = Collector::allocate<Array>();

		self->string.split([&](const CharArray &match) {
			result->vector.push(match.to_string());
		}, sep->string);

		return result;
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

	void String::initialize()
	{
		method<Arg::Self, Arg::Value>(context->string_class, "match", &match);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "split", &split);
		method<Arg::Self>(context->string_class, "inspect", &inspect);
		method<Arg::Self>(context->string_class, "to_s", &to_s);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "==", &equal);
		method<Arg::Self, Arg::Value>(context->string_class, "concat", &concat);
		method<Arg::Self, Arg::Value>(context->string_class, "<<", &concat);
		method<Arg::Self, Arg::Value>(context->string_class, "+", &concat);
	}
};

