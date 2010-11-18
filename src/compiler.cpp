#include "compiler.hpp"
#include "symbol-pool.hpp"
#include "arch/codegen.hpp"
#include "codegen/bytecode.hpp"

#include "../runtime/code_heap.hpp"

namespace Mirb
{
	Block *Compiler::compile(Tree::Scope *scope, MemoryPool &memory_pool)
	{
		CodeGen::ByteCodeGenerator generator(memory_pool);
		
		CodeGen::Block *block = generator.to_bytecode(scope);

		size_t block_size = CodeGen::NativeMeasurer::measure(block);

		void *block_code = rt_code_heap_alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate(block);
		
		return block->final;
	}
};
