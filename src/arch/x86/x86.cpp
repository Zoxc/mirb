#include "x86.hpp"

namespace Mirb
{
	namespace Arch
	{
		const size_t Register::to_real_table[registers] = {
			BX,
			SI,
			DI,
			CX,
			DX
		};

		const size_t Register::to_virtual_table[Count] = {
			None,
			3,
			4,
			0,
			None,
			None,
			1,
			2,
		};
	};
};
