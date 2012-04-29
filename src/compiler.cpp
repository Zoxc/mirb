#include "compiler.hpp"
#include "symbol-pool.hpp"
#include "block.hpp"
#include "runtime.hpp"
#include "codegen/bytecode.hpp"
#include "classes/exceptions.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "codegen/printer.hpp"
	#include "codegen/dot-printer.hpp"
#endif

namespace Mirb
{
	Block *Compiler::compile(Tree::Scope *scope, MemoryPool memory_pool)
	{
		Value::assert_valid(scope);

		CodeGen::ByteCodeGenerator generator(memory_pool);
		
		CodeGen::Block *block = generator.to_bytecode(scope);

		#ifdef MIRB_DEBUG_COMPILER
			CodeGen::ByteCodePrinter printer(block);

			std::cout << printer.print() << std::endl;
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

		block->finalize();

		return block->final;
	}

	value_t deferred_block(Frame &frame)
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
