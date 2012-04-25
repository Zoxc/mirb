#pragma once
#include "allocator.hpp"
#include <Prelude/Vector.hpp>
#include "value.hpp"

namespace Mirb
{
	class Context
	{
		public:
			value_t object_class;
			value_t class_class;
			value_t array_class;
			value_t string_class;
			value_t proc_class;
			value_t symbol_class;
			value_t nil_class;
			value_t true_class;
			value_t false_class;
			value_t module_class;
			value_t exception_class;
			value_t method_class;
			value_t standard_error;
			value_t name_error;
			value_t type_error;
			value_t argument_error;
			value_t runtime_error;
			value_t local_jump_error;
			value_t fixnum_class;
			value_t main;
			
			Context();
			
			template<typename F> void mark(F mark)
			{
				value_t *start = &object_class;

				while(start != (&main + 1))
					mark(*start++);
			}
	};

	extern Context *context;
};
