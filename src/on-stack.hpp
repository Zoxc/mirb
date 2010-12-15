#pragma once
#include "value.hpp"

namespace Mirb
{
	class CharArray;
	
	size_t *on_stack_reference(Object *&arg);
	size_t *on_stack_reference(Module *&arg);
	size_t *on_stack_reference(Class *&arg);
	size_t *on_stack_reference(Symbol *&arg);
	size_t *on_stack_reference(String *&arg);
	size_t *on_stack_reference(Array *&arg);
	size_t *on_stack_reference(Exception *&arg);
	size_t *on_stack_reference(Proc *&arg);

	size_t *on_stack_reference(value_t &arg);
	size_t *on_stack_reference(const CharArray &value);

	template<size_t size> class OnStack
	{
		public:
			template<typename Arg1> OnStack(Arg1 &&arg1)
			{
				static_assert(size == 1, "Parameter count mismatch");

				on_stack_reference(std::forward<Arg1>(arg1));
			}

			template<typename Arg1, typename Arg2> OnStack(Arg1 &&arg1, Arg2 &&arg2)
			{
				static_assert(size == 2, "Parameter count mismatch");
				
				on_stack_reference(std::forward<Arg1>(arg1));
				on_stack_reference(std::forward<Arg2>(arg2));
			}

			template<typename Arg1, typename Arg2, typename Arg3> OnStack(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3)
			{
				static_assert(size == 3, "Parameter count mismatch");
				
				on_stack_reference(std::forward<Arg1>(arg1));
				on_stack_reference(std::forward<Arg2>(arg2));
				on_stack_reference(std::forward<Arg2>(arg3));
			}
	};
};
