#include "method.hpp"
#include "runtime.hpp"
#include "generic/executable-heap.hpp"
#include "codegen/bytecode.hpp"
#include "codegen/opcodes.hpp"

#ifdef MIRB_DEBUG_BRIDGE
	#include "codegen/printer.hpp"
#endif

#ifdef MIRB_GRAPH_BRIDGE
	#include "codegen/dot-printer.hpp"
#endif

namespace Mirb
{
	namespace Arg
	{
		var_t Self::gen(MethodGen &g)
		{
			if(!g.self_arg)
				g.self_arg = g.g->create_var();

			return g.self_arg;
		}
	
		var_t Block::gen(MethodGen &g)
		{
			if(!g.block_arg)
				g.block_arg = g.g->create_var();

			return g.block_arg;
		}
	
		var_t Count::gen(MethodGen &g)
		{
			if(!g.argc_arg)
				g.argc_arg = g.g->create_var();

			return g.argc_arg;
		}
	
		var_t Values::gen(MethodGen &g)
		{
			if(!g.argv_arg)
				g.argv_arg = g.g->create_var();

			return g.argv_arg;
		}

		var_t Value::gen(MethodGen &g)
		{
			var_t var = g.g->create_var();
			g.g->gen<CodeGen::LookupOp>(var, Values::gen(g), g.argv_index++);
			return var;
		}
	};
	
	
	MethodGen::MethodGen(size_t flags, value_t module, Symbol *name, void *function, size_t arg_count) :
		flags(flags),
		module(module),
		name(name),
		function(function),
		arg_count(arg_count),
		self_arg(0),
		name_arg(0),
		module_arg(0),
		block_arg(0),
		argc_arg(0),
		argv_arg(0),
		argv_index(0)
	{
		if((flags & Method::Static) == 0)
		{
			index = 1;
			this->arg_count++;
			args = new (memory_pool) var_t[this->arg_count];
			args[0] = Arg::Self::gen(*this);
		}
		else
		{
			index = 0;
			args = new (memory_pool) var_t[arg_count];
		}
		
		g = new (memory_pool) CodeGen::ByteCodeGenerator(memory_pool);
		block = g->create();
		prolog = g->gen(g->create_block());
		g->gen<CodeGen::ReturnOp>();
		body = g->split(g->create_block());

		block->epilog = g->create_block();
		block->epilog->next_block = 0;
		body->next(block->epilog);
	}

	Block *MethodGen::gen()
	{
		CodeGen::BasicBlock *raise = g->create_block();
		body->branch(raise);
		raise->next(block->epilog);

		var_t return_var = g->create_var();
		block->final->return_var = return_var;
		/*
		g->gen<CodeGen::StaticCallOp>(return_var, function, args, arg_count);
		
		 basic->branches.push(BasicBlock::BranchInfo(gen<BranchUnlessOp>(var), lfalse));
		g->gen<CodeGen::BranchUnlessZeroOp>(block->epilog, return_var);
		*/
		g->gen(raise);
		g->gen<CodeGen::RaiseOp>();
		
		g->gen(block->epilog);
		g->gen<CodeGen::ReturnOp>();

		g->basic = prolog;
		
		#ifdef MIRB_DEBUG_BRIDGE
			CodeGen::ByteCodePrinter printer(block);

			std::cout << printer.print();
			
		#endif
		
		#ifdef MIRB_GRAPH_BRIDGE
			CodeGen::DotPrinter dot_printer;
			
			std::system("mkdir bytecode");
			
			dot_printer.print_block(block, "bytecode/bytecode.dot");

			std::stringstream path;
				
			path << "bytecode/bridge-" << name->get_string() << "-" << block->final;
				
			std::system(("mkdir \"" + path.str() + "\"").c_str());
			std::system(("dot -Tpng bytecode/bytecode.dot -o " + path.str() + ".png").c_str());
				
			for(auto i = block->variable_list.begin(); i != block->variable_list.end(); ++i)
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
			
		block->finalize();

		Block *final = block->final;
		
		final->scope = 0;
		final->name = name;
		
		
		if((flags & Method::Singleton) != 0)
			module = singleton_class(module);

		#ifdef DEBUG
			std::cout << "Defining method " << inspect_object(module) << "." << name->get_string() << "\n";
		#endif
		
		set_method(module, name, final);
		
		return block->final;
	}
	
	Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		MethodGen gen(flags, module, name, function, 0);
		return gen.gen();
	}
};
