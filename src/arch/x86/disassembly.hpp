#pragma once
#include "../../common.hpp"
#include "../../generic/vector.hpp"
#include "../../generic/memory-pool.hpp"

namespace Mirb
{
	class MemStream;

	namespace Arch
	{
		namespace Disassembly
		{
			struct Symbol {
				void *address;
				const char *symbol;
			};

			void dump_hex(unsigned char *address, int length);
			void dump_code(MemStream &stream, Vector<Symbol *, MemoryPool> *symbols);
		};
	};
};
