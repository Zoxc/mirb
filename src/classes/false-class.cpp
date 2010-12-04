#include "false-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t FalseClass::class_ref;
	
	value_t FalseClass::to_s()
	{
		return String::from_literal("false");
	}

	void FalseClass::initialize()
	{
		static_method(FalseClass::class_ref, "to_s", &to_s);

		set_const(Object::class_ref, Symbol::from_literal("FALSE"), value_false);
	};
};

