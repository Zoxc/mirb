#include "x86.hpp"

namespace Mirb
{
	namespace Arch
	{
		const size_t Registers::to_real_table[max] = {
			Register::BX,
			Register::SI,
			Register::DI,
			Register::CX,
			Register::DX
		};

		const size_t Registers::to_virtual_table[Register::Count] = {
			Register::None,
			3,
			4,
			0,
			Register::None,
			Register::None,
			1,
			2,
		};
	};
};
