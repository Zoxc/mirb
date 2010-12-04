#include "nil-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t NilClass::class_ref;
	
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
		static_method(NilClass::class_ref, "to_s", &to_s);
		static_method(NilClass::class_ref, "inspect", &inspect);

		set_const(Object::class_ref, Symbol::from_literal("NIL"), value_nil);
	};
};

