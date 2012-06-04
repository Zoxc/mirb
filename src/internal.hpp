#pragma once
#include "runtime.hpp"
#include "method.hpp"

namespace Mirb
{
	template<typename T, Class *Context::*field> value_t internal_allocate(Class *instance_of)
	{
		if(!subclass_of(context->*field, instance_of))
		{
			std::string class_name = Type::names[Type::ToTag<T>::value];
			
			CharArray instance_of_str = rescue_inspect(instance_of);
			
			raise(context->type_error, "Unable to create instance of " + instance_of_str + " with allocator for class " + CharArray(class_name));
		}
		
		return new (collector) T(instance_of);
	}
	
	template<typename T, Class *Context::*field> void internal_allocator()
	{
		singleton_method<Self<Class>, &internal_allocate<T, field>>(context->*field, "allocate");
	}
};
