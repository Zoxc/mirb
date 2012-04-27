#pragma once
#include "../common.hpp"
#include <Prelude/Vector.hpp>
#include <Prelude/List.hpp>
#include "../generic/memory-pool.hpp"
#include "opcodes.hpp"
#include "../tree/tree.hpp"

struct exception_block;

namespace Mirb
{
	class Block;
	struct ExceptionBlock;
	
	namespace Tree
	{
		class Variable;
		class Scope;
	};
	
	namespace CodeGen
	{
		class Block;
		class BasicBlock;
		class ByteCodeGenerator;
		
		class BasicBlock
		{
			public:
				BasicBlock(MemoryPool memory_pool, Block &block);

				#ifdef DEBUG
					size_t id;
				#endif

				ListEntry<BasicBlock> entry;

				std::stringstream opcodes; // TODO: Replace with own MemoryPool allocated structure
				
				typedef std::pair<size_t, BasicBlock *> BranchInfo;
				typedef std::pair<size_t, Range *> SourceInfo;
				
				Vector<BranchInfo, MemoryPool> branches;
				
				Vector<SourceInfo, MemoryPool> source_locs;
				
				size_t pos;
		};
		
		class Block
		{
			private:
				void initialize();
				size_t stack_heap;

			public:
				Mirb::Block *final;
				Tree::Scope *scope;
				BasicBlock *epilog; // The end of the block

				MemoryPool &memory_pool;
				
				/*
				 * Exception related fields
				 */
				ExceptionBlock *current_exception_block;
				Vector<ExceptionBlock *> exception_blocks;

				size_t stack_vars;
				size_t stack_heap_size;
				
				var_t self_var;
				var_t return_var;
				var_t heap_var;

				void finalize();
				
				size_t var_count; // The total variable count. May be greater than variable_list.size() when dummy variables used to flush register are added.
				
				size_t stack_alloc(size_t size);
				void stack_free(size_t address);
				
				#ifdef DEBUG
					size_t basic_block_count; // Nicer basic block labeling...
				#endif
				
				size_t loc;
				
				Block(MemoryPool &memory_pool, Tree::Scope *scope);
				Block(MemoryPool &memory_pool);
				
				List<BasicBlock> basic_blocks; // A linked list of basic blocks
		};
	};
};
