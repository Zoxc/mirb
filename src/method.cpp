#include "method.hpp"
#include "runtime.hpp"
#include "arch/codegen.hpp"
#include "generic/executable-heap.hpp"
#include "codegen/bytecode.hpp"

#ifdef DEBUG
	#include "codegen/printer.hpp"
#endif

namespace Mirb
{
	namespace Arg
	{
		Self self;
		Block block;
		Count count;
		Values values;

		void Argument::apply(MethodGen &gen) const
		{
			gen.arguments.push(this);
		}
	};

	void MethodGen::initalize(size_t flags, value_t module, Symbol *name, void *function)
	{
	}

	Block *MethodGen::gen()
	{
		CodeGen::ByteCodeGenerator generator(memory_pool);

		CodeGen::Block *block = generator.create();

		block->analyse_liveness();
		block->allocate_registers();

		#ifdef DEBUG
			CodeGen::ByteCodePrinter printer(block);

			std::cout << printer.print();
		#endif
		
		size_t block_size = CodeGen::NativeMeasurer::measure_method(block);

		void *block_code = ExecutableHeap::alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate_method(block);
		
		ExecutableHeap::resize(block_code, stream.size());

		block->final->scope = 0;
		block->final->compiled = (compiled_block_t)block_code;
		
		return block->final;
	}
};
