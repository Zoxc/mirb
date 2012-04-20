#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "object-header.hpp"
#include "generic/map.hpp"
#include "generic/vector.hpp"
#include "generic/simple-list.hpp"

struct exception_block;

namespace Mirb
{
	class Symbol;
	
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
	
	class Block: // TODO: Pin or create a dummy class which references the real one. Blocks are currently hardcoded in generated assembly.
		public ConstantHeader<Value::InternalBlock>
	{
		public:
			typedef void *compiled_t;
			
			Tree::Scope *scope;
			compiled_t compiled; // A pointer to a compiled function.
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
