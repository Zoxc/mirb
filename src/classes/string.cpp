#include "string.hpp"
#include "symbol.hpp"
#include "array.hpp"
#include "fixnum.hpp"
#include "range.hpp"
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
	
	value_t String::add(String *self, String *other)
	{
		return String::get(self->string + other->string);
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
			regexp = call_argv(context->regexp_class, "new", value_nil, 1, &regexp);

			if(!regexp || type_error(regexp, context->regexp_class))
				return 0;
		}

		return call_argv(regexp, "match", value_nil, 1, &self);
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
	
	value_t String::rb_get(String *self, value_t index)
	{
		if(Value::is_fixnum(index))
		{
			size_t i = Fixnum::to_size_t(index);

			if(i < self->string.size())
				return String::get(self->string[i]);
			else
				return String::get("");
		}
		else if(Value::of_type<Range>(index))
		{
			size_t start;
			size_t length;

			auto range = cast<Range>(index);

			if(!range->convert_to_index(start, length, self->string.size()))
				return 0;

			return String::get(self->string.copy(start, length));
		}
		else
			return type_error(index, "Range or Fixnum");
	}
	
	value_t String::ljust(String *self, size_t length, String *other)
	{
		CharArray padding = other ? other->string : " ";
		CharArray result = self->string;

		while(result.size() < length)
		{
			size_t remaining = length - self->string.size();
			result += padding.copy(0, remaining);
		}

		return result.to_string();
	}
	
	value_t to_sym(String *self)
	{
		return Symbol::get(self->string);
	}

	value_t String::compare(String *self, String *other)
	{
		for(size_t i = 0; true; ++i)
		{
			bool self_empty = self->string.size() - i == 0;
			bool other_empty = other->string.size() - i == 0;

			if(self_empty)
				return other_empty ? Fixnum::from_int(0) : Fixnum::from_int(-1);
			else if(other_empty)
				return Fixnum::from_int(1);
			
			char_t self_char = self->string[i];
			char_t other_char = other->string[i];
			
			if(self_char > other_char)
				return Fixnum::from_int(1);

			if(self_char < other_char)
				return Fixnum::from_int(-1);
		}
	}
	
	value_t String::tr(String *self, String *from, String *to)
	{
		CharArray result = self->string;

		if(!to->string.size() || !from->string.size())
			return result.to_string();

		result.each_char([&](char_t &input, size_t) {
			from->string.each_char([&](char_t &from, size_t index) {
				if(input == from)
				{
					if(index >= to->string.size())
						input = to->string[to->string.size() - 1];
					else
						input = to->string[index];
				}
			});
		});

		return result.to_string();
	}

	void String::initialize()
	{
		method<Arg::Self, Arg::Value>(context->string_class, "match", &match);
		method<Arg::Self>(context->string_class, "to_s", &to_s);
		
		method<Arg::SelfClass<String>, Arg::Value>(context->string_class, "[]", &rb_get);

		method<Arg::SelfClass<String>>(context->string_class, "downcase", &downcase);
		method<Arg::SelfClass<String>>(context->string_class, "downcase!", &downcase_self);
		method<Arg::SelfClass<String>>(context->string_class, "upcase", &upcase);
		method<Arg::SelfClass<String>>(context->string_class, "upcase!", &upcase_self);
		
		method<Arg::SelfClass<String>, Arg::UInt, Arg::DefaultClass<String>>(context->string_class, "ljust", &ljust);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "split", &split);
		method<Arg::SelfClass<String>>(context->string_class, "inspect", &inspect);
		method<Arg::SelfClass<String>>(context->string_class, "to_sym", &to_sym);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "<=>", &String::compare);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "==", &equal);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "===", &equal);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "concat", &concat);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "<<", &concat);
		method<Arg::SelfClass<String>, Arg::Class<String>>(context->string_class, "+", &add);
		method<Arg::SelfClass<String>, Arg::Class<String>, Arg::Class<String>>(context->string_class, "tr", &tr);
	}
};

