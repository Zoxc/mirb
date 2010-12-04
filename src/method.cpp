#include "method.hpp"
#include "runtime.hpp"
#include "arch/codegen.hpp"
#include "generic/executable-heap.hpp"
#include "codegen/bytecode.hpp"
#include "codegen/opcodes.hpp"

#ifdef DEBUG
	#include "codegen/printer.hpp"
#endif

namespace Mirb
{
	template<CodeGen::ArgType::Arg arg> static Tree::Variable *gen_arg(CodeGen::ByteCodeGenerator *g)
	{
		Tree::Variable *var = g->create_var();
		g->gen<CodeGen::LoadArgOp>(var, arg);
		return var;
	};
	
	namespace Arg
	{
		Tree::Variable *Self::gen(MethodGen &g)
		{
			return gen_arg<CodeGen::ArgType::Self>(g.g);
		}
	
		Tree::Variable *Block::gen(MethodGen &g)
		{
			return gen_arg<CodeGen::ArgType::Block>(g.g);
		}
	
		Tree::Variable *Count::gen(MethodGen &g)
		{
			return gen_arg<CodeGen::ArgType::Count>(g.g);
		}
	
		Tree::Variable *Values::gen(MethodGen &g)
		{
			return gen_arg<CodeGen::ArgType::Values>(g.g);
		}

		Tree::Variable *Value::gen(MethodGen &g)
		{
			Tree::Variable *var = g.g->create_var();
			g.g->gen<CodeGen::LookupOp>(var, g.get_rb_args(), g.rb_arg_index++);
			return var;
		}
	};
	
	
	MethodGen::MethodGen(size_t flags, value_t module, Symbol *name, void *function, size_t arg_count) :
		flags(flags),
		module(module),
		name(name),
		function(function),
		arg_count(arg_count),
		rb_args(0),
		rb_arg_index(0)
	{
		if((flags & Method::Static) == 0)
		{
			index = 1;
			this->arg_count++;
		}
		else
			index = 0;

		g = new (memory_pool) CodeGen::ByteCodeGenerator(memory_pool);
		args = new Tree::Variable *[arg_count];
		block = g->create();
	}

	Tree::Variable *MethodGen::get_rb_args()
	{
		if(!rb_args)
		{
			rb_args = g->create_var();
			g->gen<CodeGen::LoadArgOp>(rb_args, CodeGen::ArgType::Values);
		}

		return rb_args;
	}

	Block *MethodGen::gen()
	{
		if((flags & Method::Static) == 0)
		{
			args[0] = g->create_var();
			g->gen<CodeGen::LoadArgOp>(args[0], CodeGen::ArgType::Self);
		}

		g->gen<CodeGen::StaticCallOp>(block->return_var, function, args, arg_count);

		block->analyse_liveness();
		block->allocate_registers();

		#ifdef MIRB_DEBUG_BRIDGE
			CodeGen::ByteCodePrinter printer(block);

			std::cout << printer.print();
		#endif
		
		size_t block_size = CodeGen::NativeMeasurer::measure_method(block);

		void *block_code = ExecutableHeap::alloc(block_size);

		MemStream stream(block_code, block_size);

		CodeGen::NativeGenerator native_generator(stream, memory_pool);

		native_generator.generate_method(block);
		
		ExecutableHeap::resize(block_code, stream.size());

		Block *final = block->final;

		final->scope = 0;
		final->compiled = (compiled_block_t)block_code;
		final->name = name;

		if((flags & Method::Singleton) != 0)
			module = singleton_class(module);

		#ifdef DEBUG
			std::cout << "Defining method " << inspect_object(module) << "." << name->get_string() << "\n";
		#endif
		
		set_method(module, name, final);
		
		return final;
	}
	
	Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		MethodGen gen(flags, module, name, function, 0);
		return gen.gen();
	}
};
