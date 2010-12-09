#include "exceptions.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t StandardError::class_ref;
	
	void StandardError::initialize()
	{
		StandardError::class_ref = define_class(Object::class_ref, "StandardError", Exception::class_ref);
	}

	value_t NameError::class_ref;
	
	void NameError::initialize()
	{
		NameError::class_ref = define_class(Object::class_ref, "NameError", StandardError::class_ref);
	}
};

