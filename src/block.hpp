#pragma once
#include "common.hpp"
#include "map.hpp"
#include "vector.hpp"
#include "simple-list.hpp"
#include "arch/block.hpp"

struct exception_block;

namespace Mirb
{
	namespace CodeGen
	{
		class BasicBlock;
		struct BreakTargetOp;
	};

	namespace Tree
	{
		class Scope;
	};
	
	enum ExceptionHandlerType {
		RuntimeException,
		ClassException,
		FilterException,
	};

	struct ExceptionHandler {
		ExceptionHandlerType type;
	};
	
	union BlockLabel
	{
		CodeGen::BasicBlock *block;
		void *address;
	};
	
	struct RuntimeExceptionHandler:
		public ExceptionHandler
	{
		BlockLabel rescue_label;
	};

	struct ClassExceptionHandler:
		public RuntimeExceptionHandler
	{
	};

	struct ExceptionBlock {
		size_t parent_index;
		ExceptionBlock *parent;
		Vector<ExceptionHandler *> handlers;
		BlockLabel ensure_label;
	};
	
	enum ExceptionType {
		RubyException,
		ReturnException,
		BreakException,
		ThrowException
	};

	class Block;

	struct ExceptionData {
		ExceptionType type;
		Block *target;
		union
		{
			struct
			{
				value_t value;
			} break_data;
		};
		void *payload[3];
	};

	class Block
	{
		public:
			Tree::Scope *scope;
			compiled_block_t compiled; // A pointer to a compiled function.
			Symbol *name; // The name of this block.
			
			Vector<ExceptionBlock *> exception_blocks;
			void **break_targets;
			size_t ebp_offset;
			void *epilog;
			
			Vector<Block *> blocks; // A list of child blocks so the GC won't free them.
	};
	
	class BlockMapFunctions:
		public MapFunctions<Symbol *, Block *>
	{
		public:
			static Block *invalid_value()
			{
				return 0;
			}
	};

	typedef Map<Symbol *, Block *, GC, BlockMapFunctions> BlockMap;
};
