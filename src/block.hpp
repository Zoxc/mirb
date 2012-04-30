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
		FilterException,
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

	struct ExceptionBlock {
		ExceptionBlock *parent;
		Vector<ExceptionHandler *> handlers;
		BlockLabel ensure_label;

		~ExceptionBlock()
		{
			for(auto handler: handlers)
				delete handler;
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
				ranges(nullptr),
				source_location(2),
				break_targets(nullptr)
			{
			}
			
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

				if(exception_blocks)
				{
					for(size_t i = 0; i < exception_block_count; ++i)
						delete exception_blocks[i];

					delete[] exception_blocks;
				}
			};

			typedef value_t (*executor_t)(Frame &frame);

			Tree::Scope *scope;
			Document *document;

			size_t var_words;

			const char *opcodes;

			ExceptionBlock **exception_blocks;
			size_t exception_block_count;

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
