#pragma once
#include <Prelude/Map.hpp>
#include <Prelude/Vector.hpp>
#include <Prelude/List.hpp>
#include "common.hpp"
#include "gc.hpp"
#include "vm.hpp"
#include "object-header.hpp"

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
		size_t address;
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
		ExceptionBlock *parent;
		Vector<ExceptionHandler *> handlers;
		BlockLabel ensure_label;
	};
	
	typedef size_t var_t;
	
	extern const var_t no_var;

	class Range;
	class Document;

	class Block: // TODO: Pin or create a dummy class which references the real one. Blocks are currently hardcoded in generated assembly.
		public ConstantHeader<Value::InternalBlock>
	{
		public:
			Block(Document *document) : document(document), source_location(2) {}

			typedef value_t (*executor_t)(Frame &frame);

			Tree::Scope *scope;
			Document *document;

			Symbol *name; // The name of this block.
			
			size_t var_words;

			const char *opcodes;

			Map<size_t, Range *> source_location;

			executor_t executor;
			
			var_t *break_targets;
			
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
