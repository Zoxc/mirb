#pragma once
#include "value.hpp"

namespace Mirb
{
	class Object;
	class CharArray;

	template<typename T> size_t *on_stack_reference(T arg)
	{
		(void)static_cast<Object *>(arg);

		return (size_t *)&arg;
	}
	
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
	};
};
