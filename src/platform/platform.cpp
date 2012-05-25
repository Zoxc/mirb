#include "platform.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	namespace Platform
	{
		CharArray expand_path(CharArray relative)
		{
			if(!File::absolute_path(relative))
				return File::join(cwd(), relative);
			else
				return relative;
		};
	
	};
};
