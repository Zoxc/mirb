#include "compiler.hpp"
#include "symbol-pool.hpp"
#include "gc.hpp"
#include "block.hpp"
#include "generic/executable-heap.hpp"
#include "codegen/bytecode.hpp"

#ifdef DEBUG
	#include "codegen/printer.hpp"
	#include "codegen/dot-printer.hpp"
#endif

namespace Mirb
{
	Block *Compiler::compile(Tree::Scope *scope, MemoryPool &memory_pool)
	{
		CodeGen::ByteCodeGenerator generator(memory_pool);
		
		CodeGen::Block *block = generator.to_bytecode(scope);
		
		#ifdef DEBUG
			CodeGen::ByteCodePrinter printer(block);

			std::cout << printer.print();
		#endif
		
		#ifdef MIRB_GRAPH_BYTECODE
			CodeGen::DotPrinter dot_printer;
			
			std::system("mkdir bytecode");
				
			dot_printer.print_block(block, "bytecode/bytecode.dot");

			std::stringstream path;
				
			path << "bytecode/block-" << block->final;
				
			std::system(("mkdir \"" + path.str() + "\"").c_str());
			std::system(("dot -Tpng bytecode/bytecode.dot -o " + path.str() + ".png").c_str());
		#endif
/*
		size_t block_size = CodeGen::NativeMeasurer::measure(block);

		void *block_code = ExecutableHeap::alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate(block);
		
		ExecutableHeap::resize(block_code, stream.size());

		block->final->scope = 0;
		block->final->compiled = (Block::compiled_t)block_code;
		*/
		return block->final;
	}

	Block *Compiler::defer(Tree::Scope *scope, MemoryPool &memory_pool)
	{
		Block *block = Collector::allocate<Block>();
		
		block->scope = scope;

		scope->final = block;
		/*
		size_t block_size = CodeGen::NativeGenerator::stub_size;

		void *block_code = ExecutableHeap::alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate_stub(block);

		block->compiled = (Block::compiled_t)block_code;
		*/
		return block;
	}

	Block::compiled_t Compiler::defered_compile(Block *block)
	{
		MemoryPool memory_pool;

		compile(block->scope, memory_pool);
		
		block->scope = 0;

		return block->compiled;
	}
};
