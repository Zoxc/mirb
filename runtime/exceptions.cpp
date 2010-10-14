#include "../globals.hpp"
#include "exceptions.hpp"

#ifndef WIN_SEH
	__thread struct rt_frame *rt_current_frame = 0;
#endif

void __attribute__((noreturn)) rt_exception_raise(struct rt_exception_data *data)
{
	#ifdef WIN_SEH
		RaiseException(RT_SEH_RUBY, EXCEPTION_NONCONTINUABLE, 1, (const DWORD *)&data);

		__builtin_unreachable();
	#else
		struct rt_frame *top = rt_current_frame;
		struct rt_frame *frame = top;

		while(frame)
		{
			frame->handler(frame, top, data, false);
			frame = frame->prev;
		}

		RT_ASSERT(0);
	#endif
}

