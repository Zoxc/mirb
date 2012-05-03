#include "method.hpp"
#include "runtime.hpp"
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
		Self::type Self::apply(Frame &frame, State &)
		{
			return frame.obj;
		}
		
		Block::type Block::apply(Frame &frame, State &)
		{
			return frame.block;
		}

		Module::type Module::apply(Frame &frame, State &)
		{
			return frame.module;
		}

		Count::type Count::apply(Frame &frame, State &)
		{
			return frame.argc;
		}

		Values::type Values::apply(Frame &frame, State &)
		{
			return frame.argv;
		}

		Value::type Value::apply(Frame &frame, State &state)
		{
			return frame.argv[state.index++];
		}
	}

	value_t wrapper(Frame &frame)
	{
		return ((value_t (*)())frame.code->opcodes)();
	}

	Block *generate_block(size_t flags prelude_unused, Module *module, Symbol *name, Block::executor_t executor, void *function)
	{
		Block *result = Collector::allocate_pinned<Block>(nullptr);

		Value::assert_valid(module);

		result->opcodes = (const char *)function;
		result->executor = executor;
		
		#ifdef DEBUG
			OnStack<3> os(module, name, result);

			std::cout << "Defining method " << inspect_object(module) << "." << name->get_string() << "\n";
		#endif

		module->set_method(name, Collector::allocate<Method>(result, module));

		return result;
	}

	Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
	{
		return generate_block(flags, module, name, wrapper, function);
	}
};
