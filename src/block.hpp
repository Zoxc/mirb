#pragma once
#include "common.hpp"
#include <Prelude/Map.hpp>
#include <Prelude/Vector.hpp>
#include <Prelude/List.hpp>
#include "generic/source-loc.hpp"
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
		StandardException,
		FilterException
	};
	
	union BlockLabel
	{
		CodeGen::Label *label;
		size_t address;
	};
	
	typedef size_t var_t;
	
	struct ExceptionHandler {
		ExceptionHandlerType type;
		BlockLabel rescue_label;
		var_t var;
	};
	
	struct FilterExceptionHandler:
		public ExceptionHandler
	{
		BlockLabel test_label;
		var_t result;
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
	
	extern const var_t no_var;

	class SourceLoc;
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

			static const bool finalizer = true;

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
			
			SourceLoc *range;
			SourceLoc *ranges;
			Map<size_t, SourceLoc *> source_location;

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
