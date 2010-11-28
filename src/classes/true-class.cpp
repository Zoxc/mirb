#include "true-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t TrueClass::class_ref;
	
	mirb_compiled_block(trueclass_to_s)
	{
		return String::from_literal("true");
	}

	void TrueClass::initialize()
	{
		TrueClass::class_ref = define_class(Object::class_ref, "TrueClass", Object::class_ref);

		define_method(TrueClass::class_ref, "to_s", trueclass_to_s);

		set_const(Object::class_ref, Symbol::from_literal("TRUE"), value_true);
	};
};

