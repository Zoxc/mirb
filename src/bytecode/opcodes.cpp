#include "opcodes.hpp"

namespace Mirb
{
	OpcodeBlock *OpcodeBlock::allocate()
	{
		unsigned char *memory = (unsigned char *)rt_alloc(sizeof(OpcodeBlock) + block_size);
		
		OpcodeBlock *result = new (memory) OpcodeBlock;
		
		result->block_start = (Opcode *)memory + block_size;
		result->block_current = memory + block_size;
		
		return result;
	}
};
