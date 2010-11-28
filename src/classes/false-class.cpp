#include "false-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t FalseClass::class_ref;
	
	mirb_compiled_block(falseclass_to_s)
	{
		return String::from_literal("false");
	}

	void FalseClass::initialize()
	{
		FalseClass::class_ref = define_class(Object::class_ref, "FalseClass", Object::class_ref);

		define_method(FalseClass::class_ref, "to_s", falseclass_to_s);

		set_const(Object::class_ref, Symbol::from_literal("FALSE"), value_false);
	};
};

