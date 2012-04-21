#include "exceptions.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t StandardError::class_ref;
	value_t NameError::class_ref;
	value_t TypeError::class_ref;
	value_t ArgumentError::class_ref;
	value_t RuntimeError::class_ref;
	value_t LocalJumpError::class_ref;
	
	void initialize_exceptions()
	{
		StandardError::class_ref = define_class(Object::class_ref, "StandardError", Exception::class_ref);
		NameError::class_ref = define_class(Object::class_ref, "NameError", StandardError::class_ref);
		TypeError::class_ref = define_class(Object::class_ref, "TypeError", StandardError::class_ref);
		ArgumentError::class_ref = define_class(Object::class_ref, "ArgumentError", StandardError::class_ref);
		RuntimeError::class_ref = define_class(Object::class_ref, "RuntimeError", StandardError::class_ref);
		LocalJumpError::class_ref = define_class(Object::class_ref, "LocalJumpError", StandardError::class_ref);
	}
};

