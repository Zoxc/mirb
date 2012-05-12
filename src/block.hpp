#pragma once
#include "common.hpp"
#include <Prelude/Map.hpp>
#include <Prelude/Vector.hpp>
#include <Prelude/List.hpp>
#include "generic/range.hpp"
#include "vm.hpp"
#include "allocator.hpp"

struct exception_block;

namespace Mirb
{
	class Symbol;
	
	namespace CodeGen
	{
		class Label;
		struct BreakTargetOp;
	};

	namespace Tree
	{
		class Scope;
	};
	
	enum ExceptionHandlerType {
		RuntimeException,
		ClassException,
		FilterException
	};

	struct ExceptionHandler {
		ExceptionHandlerType type;
	};
	
	union BlockLabel
	{
		CodeGen::Label *label;
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
	
	struct LoopHandler
	{
		BlockLabel next_label;
		BlockLabel break_label;
	};

	struct ExceptionBlock {
		ExceptionBlock *parent;
		Vector<ExceptionHandler *> handlers;
		BlockLabel ensure_label;
		LoopHandler *loop;

		~ExceptionBlock()
		{
			for(auto handler: handlers)
				delete handler;

			if(loop)
				delete loop;
		}
	};
	
	typedef size_t var_t;
	
	extern const var_t no_var;

	class Range;
	class Document;

	class Block:
		public PinnedHeader
	{
		public:
			Block(Document *document) :
				PinnedHeader(Value::InternalBlock),
				scope(nullptr),
				document(document),
				opcodes(nullptr),
				exception_blocks(nullptr),
				strings(nullptr),
				range(nullptr),
				ranges(nullptr),
				source_location(2),
				break_targets(nullptr)
			{
			}
			
			~Block()
			{
				if(range)
					delete range;

				if(ranges)
					std::free(ranges);

				if(break_targets)
					delete[] break_targets;

				if(executor == evaluate_block)
					std::free((void *)opcodes);

				if(strings)
				{
					for(size_t i = 0; i < string_count; ++i)
						std::free((void *)strings[i]);

					delete[] strings;
				}

				if(exception_blocks)
				{
					for(size_t i = 0; i < exception_block_count; ++i)
						delete exception_blocks[i];

					delete[] exception_blocks;
				}
			};

			typedef value_t (*executor_t)(Frame &frame);

			union
			{
				Tree::Scope *scope;
				Symbol *symbol;
			};

			Document *document;
			size_t var_words;

			union
			{
				const char *opcodes;
				void *function;
			};

			size_t min_args;
			size_t max_args;

			ExceptionBlock **exception_blocks;
			size_t exception_block_count;

			const char_t **strings;
			size_t string_count;
			
			Range *range;
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
