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
	
	value_t RuntimeError::class_ref;
	
	void RuntimeError::initialize()
	{
		RuntimeError::class_ref = define_class(Object::class_ref, "RuntimeError", StandardError::class_ref);
	}
	
	value_t LocalJumpError::class_ref;
	
	void LocalJumpError::initialize()
	{
		LocalJumpError::class_ref = define_class(Object::class_ref, "LocalJumpError", StandardError::class_ref);
	}
};

