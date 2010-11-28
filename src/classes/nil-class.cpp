#include "nil-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t NilClass::class_ref;

	mirb_compiled_block(nilclass_to_s)
	{
		return String::from_literal("");
	}

	mirb_compiled_block(nilclass_inspect)
	{
		return String::from_literal("nil");
	}
	
	void NilClass::initialize()
	{
		NilClass::class_ref = define_class(Object::class_ref, "NilClass", Object::class_ref);

		define_method(NilClass::class_ref, "inspect", nilclass_inspect);
		define_method(NilClass::class_ref, "to_s", nilclass_to_s);

		set_const(Object::class_ref, Symbol::from_literal("NIL"), value_nil);
	};
};

