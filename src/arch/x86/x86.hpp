#pragma once
#include "../../common.hpp"

namespace Mirb
{
	namespace Arch
	{
		const size_t registers  = 5;
		
		class Register
		{
			public:
				enum Reg
				{
					AX,
					CX,
					DX,
					BX,
					SP,
					BP,
					SI,
					DI,

					Count,
					None = -1
				};
				
				static const size_t to_real_table[registers];
				static const size_t to_virtual_table[Count];

				static size_t to_real(size_t loc)
				{
					return to_real_table[loc];
				}
				
				static size_t to_virtual(size_t reg)
				{
					return to_virtual_table[reg];
				}
				
		};
		
		const size_t caller_saved[2] = {
			Register::CX,
			Register::DX
		};
	};
};
