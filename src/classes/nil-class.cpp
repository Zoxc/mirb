#include "nil-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t NilClass::to_s()
	{
		return String::from_literal("");
	}

	value_t NilClass::inspect()
	{
		return String::from_literal("nil");
	}
	
	void NilClass::initialize()
	{
		static_method(context->nil_class, "to_s", &to_s);
		static_method(context->nil_class, "inspect", &inspect);

		set_const(context->object_class, Symbol::from_literal("NIL"), value_nil);
	};
};

