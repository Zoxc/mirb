#pragma once
#include <functional>

namespace Mirb
{
	struct Finally
	{
		std::function<void()> func;

		template<typename F> Finally(F func) : func(func)
		{
		}

		~Finally()
		{
			func();
		}
	};
};
