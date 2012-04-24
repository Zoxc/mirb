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
		Self::type Self::apply(Frame &frame, State &state)
		{
			return frame.obj;
		}
		
		Block::type Block::apply(Frame &frame, State &state)
		{
			return frame.block;
		}

		Count::type Count::apply(Frame &frame, State &state)
		{
			return frame.argc;
		}

		Values::type Values::apply(Frame &frame, State &state)
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

	Block *generate_block(size_t flags, value_t module, Symbol *name, Block::executor_t executor, void *function)
	{
		Block *result = Collector::allocate_pinned<Block>(nullptr);

		result->opcodes = (const char *)function;
		result->name = name;
		result->executor = executor;
		
		if((flags & Method::Singleton) != 0)
			module = singleton_class(module);

		#ifdef DEBUG
			std::cout << "Defining method " << inspect_object(module) << "." << name->get_string() << "\n";
		#endif
		
		set_method(module, name, result);

		return result;
	}

	Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		return generate_block(flags, module, name, wrapper, function);
	}
};
