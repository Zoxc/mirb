#include "platform.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	namespace Platform
	{
		value_t Exception::raise() const
		{
			return Mirb::raise(context->system_call_error, message);
		}

		CharArray expand_path(CharArray relative)
		{
			if(!File::absolute_path(relative))
				return File::join(cwd(), relative);
			else
				return relative;
		};
	
	};
};
