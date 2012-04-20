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
	
	typedef size_t var_t;
	
	extern const var_t no_var;

	class Block: // TODO: Pin or create a dummy class which references the real one. Blocks are currently hardcoded in generated assembly.
		public ConstantHeader<Value::InternalBlock>
	{
		public:
			Block();

			Tree::Scope *scope;
			Symbol *name; // The name of this block.
			
			var_t heap_array_var;
			var_t heap_var;
			var_t self_var;
			var_t return_var;

			size_t var_words;

			const char *opcodes;
				
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
