#include "compiler.hpp"
#include "symbol-pool.hpp"
#include "gc.hpp"
#include "block.hpp"
#include "arch/codegen.hpp"
#include "codegen/bytecode.hpp"

#include "../runtime/code_heap.hpp"

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

		block->analyse_liveness();
		block->allocate_registers();

		#ifdef DEBUG
			CodeGen::DotPrinter dot_printer;
			CodeGen::ByteCodePrinter printer(block);

			std::cout << printer.print();

			std::system("mkdir bytecode");
				
			dot_printer.print_block(block, "bytecode/bytecode.dot");

			std::stringstream path;
				
			path << "bytecode/block-" << block;
				
			std::system(("mkdir \"" + path.str() + "\"").c_str());
			std::system(("dot -Tpng bytecode/bytecode.dot -o " + path.str() + ".png").c_str());
				
			for(auto i = scope->variable_list.begin(); i != scope->variable_list.end(); ++i)
			{
				if(i()->flags.get<Tree::Variable::FlushCallerSavedRegisters>())
					continue;

				dot_printer.highlight = *i;

				dot_printer.print_block(block, "bytecode/bytecode.dot");
					
				std::stringstream result;

				result << "dot -Tpng bytecode/bytecode.dot -o " << path.str() << "/var" << i()->index << ".png";

				std::system(result.str().c_str());
			}
		#endif

		size_t block_size = CodeGen::NativeMeasurer::measure(block);

		void *block_code = rt_code_heap_alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate(block);

		block->final->scope = 0;
		block->final->compiled = (compiled_block_t)block_code;
		
		return block->final;
	}

	Block *Compiler::defer(Tree::Scope *scope, MemoryPool &memory_pool)
	{
		Block *block = new (gc) Block;
		
		block->scope = scope;

		scope->final = block;
		
		size_t block_size = CodeGen::NativeGenerator::stub_size;

		void *block_code = rt_code_heap_alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate_stub(block);

		block->compiled = (compiled_block_t)block_code;
		
		return block;
	}

	compiled_block_t Compiler::defered_compile(Block *block)
	{
		MemoryPool memory_pool;

		compile(block->scope, memory_pool);
		
		block->scope = 0;

		return block->compiled;
	}
};
