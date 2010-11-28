#include "string.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t String::class_ref;

	value_t String::from_symbol(Symbol *symbol)
	{
		return auto_cast(new (gc) String(symbol->string));
	}
	
	value_t String::from_string(const char *c_str)
	{
		return auto_cast(new (gc) String((const char_t *)c_str, std::strlen(c_str)));
	}
	
	mirb_compiled_block(string_inspect)
	{
		auto self = cast<String>(obj);

		CharArray result = "\"" + self->string + "\"";

		return result.to_string();
	}

	mirb_compiled_block(string_to_s)
	{
		return obj;
	}

	mirb_compiled_block(string_concat)
	{
		auto self = cast<String>(obj);
		auto other = cast<String>(MIRB_ARG(0));

		self->string.append(other->string);

		return obj;
	}

	void String::initialize()
	{
		define_method(String::class_ref, "inspect", string_inspect);
		define_method(String::class_ref, "to_s", string_to_s);
		define_method(String::class_ref, "concat", string_concat);
		define_method(String::class_ref, "<<", string_concat);
		define_method(String::class_ref, "+", string_concat);
	}
};

