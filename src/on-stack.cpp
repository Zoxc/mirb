#include "on-stack.hpp"

namespace Mirb
{
	size_t *on_stack_reference(Object *&arg)
	{
		return (size_t *)&arg;
	}
	
	size_t *on_stack_reference(Module *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(Class *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(Symbol *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(String *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(Array *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(Exception *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(Proc *&arg)
	{
		return (size_t *)&arg;
	}

	size_t *on_stack_reference(value_t &arg)
	{
		return &arg;
	}

	size_t *on_stack_reference(const CharArray &value)
	{
		return 0;
	}
};
