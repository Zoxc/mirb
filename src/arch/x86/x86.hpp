#pragma once
#include "../../common.hpp"

namespace Mirb
{
	namespace Arch
	{
		static const size_t registers = 5;
		
		class Register
		{
			public:
				enum Reg
				{
					eax,
					ebx,
					ecx,
					edx,
					esi,
					edi,
					esp,
					ebp
				};
		};
	};
};
