#pragma once
#include "../../common.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		class Block;
	};

	namespace Arch
	{
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
		};
		
		struct StackArg
		{
			enum Type
			{
				Self,
				MethodName,
				Count,
				Values
			};
		};
		
		class Registers
		{
			public:
				static const size_t max = 5;
				static const size_t count = max;

				Registers(CodeGen::Block &block) {}

				static size_t to_real(size_t loc)
				{
					return to_real_table[loc];
				}
				
				static size_t to_virtual(size_t reg)
				{
					return to_virtual_table[reg];
				}

			private:
				static const size_t to_real_table[max];
				static const size_t to_virtual_table[Register::Count];
		};
		
		const size_t caller_saved[2] = {
			Register::CX,
			Register::DX
		};
	};
};
