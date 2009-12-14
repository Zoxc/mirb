#include "../globals.h"
#include "exceptions.h"

#ifdef WIN32
	void __attribute__((noreturn)) rt_exception_raise(rt_value exception)
	{
		RaiseException(RT_SEH_RUBY + E_RUBY_EXCEPTION, 0, 1, (const DWORD *)exception);

		__builtin_unreachable();
	}
#else
	__thread struct rt_frame *rt_current_handler;
#endif
