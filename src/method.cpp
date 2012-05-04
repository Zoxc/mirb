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
		Info &Info::operator +=(const Info &other)
		{
			min += other.min;
			max += other.max;
			any_arg = any_arg || other.any_arg;

			return *this;
		}

		const Info Self::info = {0, 0, false};
		const Info Block::info = {0, 0, false};
		const Info Count::info = {0, 0, true};
		const Info Values::info = {0, 0, false};
		const Info Value::info = {1, 1, false};
		const Info Default::info = {0, 1, false};

		Self::type Self::apply(Frame &frame, State &)
		{
			return frame.obj;
		}
		
		Block::type Block::apply(Frame &frame, State &)
		{
			return frame.block;
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

		Value::type Default::apply(Frame &frame, State &state)
		{
			if(state.index >= frame.argc)
				return value_raise;
			else
				return frame.argv[state.index++];
		}
	}
	
	namespace MethodGen
	{
		Arg::Info fold(size_t num, ...)
		{
			va_list ap;
			Arg::Info value = {0, 0, false};
 
			va_start(ap, num);

			for(size_t i = 0; i < num; ++i)
				value += va_arg(ap, Arg::Info);

			va_end(ap);

			return value;
		};
	
		value_t wrapper(Frame &frame)
		{
			Arg::State state(frame, fold(0));

			if(state.error)
				return value_raise;
		
			return ((value_t (*)())frame.code->opcodes)();
		}

		Block *generate_block(size_t flags prelude_unused, Module *module, Symbol *name, Arg::Info &&info, Block::executor_t executor, void *function)
		{
			Block *result = Collector::allocate_pinned<Block>(nullptr);

			Value::assert_valid(module);

			result->opcodes = (const char *)function;
			result->executor = executor;
			result->min_args = info.min;
			result->max_args = info.any_arg ? (size_t)-1 : info.max;
		
			Tuple<Module> *scope = Tuple<Module>::allocate(1);

			(*scope)[0] = module;

			module->set_method(name, Collector::allocate<Method>(result, scope));

			return result;
		}

		Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(0), wrapper, function);
		}
	};
};
