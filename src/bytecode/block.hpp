#pragma once
#include "../../runtime/runtime.hpp"
#include "../common.hpp"
#include "../simple-list.hpp"
#include "opcodes.hpp"

namespace Mirb
{
	class Block;
	
	class Variable
	{
		public:
			enum Type
			{
				Temporary,
				Local,
				Stack,
				Heap,
				Types
			};
			
			Type type;
			size_t index;
	};
	
	class LocalVariable:
		public Variable
	{
		public:
			Block *owner;
			rt_value name;
	};
	
	class Block
	{
		Block *parent; // The block enclosing this one
		Block *owner; // The first parent that isn't a closure. This field can point to itself.
		
		SimpleList<OpcodeBlock> opcode_blocks; // A linked list of opcode blocks
	}
};
