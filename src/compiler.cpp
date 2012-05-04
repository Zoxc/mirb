#include "compiler.hpp"
#include "symbol-pool.hpp"
#include "block.hpp"
#include "runtime.hpp"
#include "codegen/bytecode.hpp"
#include "classes/exceptions.hpp"

namespace Mirb
{
	Block *Compiler::compile(Tree::Scope *scope, MemoryPool memory_pool)
	{
		Value::assert_valid(scope);

		CodeGen::ByteCodeGenerator generator(memory_pool, scope);
		
		return generator.generate();
	}

	value_t Compiler::deferred_block(Frame &frame)
	{
		{
			MemoryPool::Base memory_pool;

			Compiler::compile(frame.code->scope, memory_pool);
		}

		return frame.code->executor(frame);
	}

	Block *Compiler::defer(Tree::Scope *scope)
	{
		Block *block = Collector::allocate_pinned<Block>(scope->document);
		
		scope->final = block;
		block->scope = scope;
		block->executor = deferred_block;

		return block;
	}
};
