#include "true-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t TrueClass::to_s()
	{
		return String::get("true");
	}

	void TrueClass::initialize()
	{
		method(context->true_class, "to_s", &to_s);

		set_const(context->object_class, Symbol::get("TRUE"), value_true);
	};
};

