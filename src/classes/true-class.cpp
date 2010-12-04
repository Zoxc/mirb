#include "true-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t TrueClass::class_ref;
	
	value_t TrueClass::to_s()
	{
		return String::from_literal("true");
	}

	void TrueClass::initialize()
	{
		static_method(TrueClass::class_ref, "to_s", &to_s);

		set_const(Object::class_ref, Symbol::from_literal("TRUE"), value_true);
	};
};

