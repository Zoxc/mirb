#pragma once
#include "common.hpp"
#include <Prelude/Map.hpp>
#include <Prelude/Vector.hpp>
#include <Prelude/List.hpp>
#include "vm.hpp"
#include "allocator.hpp"

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

	class Block:
		public PinnedHeader
	{
		public:
			Block(Document *document) : PinnedHeader(Value::InternalBlock), scope(nullptr), document(document), opcodes(nullptr), strings(nullptr), ranges(nullptr), source_location(2) {}
			
			~Block()
			{
				if(ranges)
					std::free(ranges);

				if(break_targets)
					delete[] break_targets;

				if(executor == evaluate_block)
					std::free((void *)opcodes);

				if(strings)
				{
					for(size_t i = 0; i < string_count; ++i)
						delete[] strings[i];

					delete[] strings;
				}
			};

			typedef value_t (*executor_t)(Frame &frame);

			Tree::Scope *scope;
			Document *document;

			size_t var_words;

			const char *opcodes;

			const char_t **strings;
			size_t string_count;

			Range *ranges;
			Map<size_t, Range *> source_location;

			executor_t executor;
			
			var_t *break_targets;
			
			Vector<Block *> blocks; // A list of child blocks so the GC won't free them.

			template<typename F> void mark(F mark)
			{
				if(scope)
					mark(scope);

				blocks.mark_content(mark);

				if(document)
					mark(document);
			}
	};
};
