#include "method.hpp"
#include "runtime.hpp"
#include "codegen/bytecode.hpp"
#include "codegen/opcodes.hpp"
#include "classes/fixnum.hpp"

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

		const Info Block::info = {0, 0, false};
		const Info Count::info = {0, 0, true};
		const Info Values::info = {0, 0, false};

		Block::Type Block::apply(Frame &frame, State &)
		{
			return frame.block;
		}

		Count::Type Count::apply(Frame &frame, State &)
		{
			return frame.argc;
		}

		Values::Type Values::apply(Frame &frame, State &)
		{
			return frame.argv;
		}
		
		Fixnum::Type Fixnum::coerce(value_t value, State &state)
		{
			if(Mirb::Value::is_fixnum(value))
				return Mirb::Fixnum::to_int(value);
			else
			{
				state.error = true;
				type_error(value, "Fixnum");
				return 0;
			}
		}
		
		UInt::Type UInt::coerce(value_t value, State &state)
		{
			if(Mirb::Value::is_fixnum(value))
			{
				auto result = Mirb::Fixnum::to_int(value);

				if(result < 0)
				{
					state.error = true;
					raise(context->standard_error, "A value above zero was expected.");
					return 0;
				}
				else
					return result;
			}
			else
			{
				state.error = true;
				type_error(value, "Fixnum above zero");
				return 0;
			}
		}
		
		const value_t Value::default_value = 0;

		Value::Type Value::coerce(value_t value, State &state)
		{
			return value;
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
		
			return ((value_t (*)())frame.code->function)();
		}

		Method *generate_block(size_t flags prelude_unused, Module *module, Symbol *name, Arg::Info &&info, Block::executor_t executor, void *function)
		{
			Block *block = Collector::allocate_pinned<Block>(nullptr);

			Value::assert_valid(module);

			block->function = function;
			block->executor = executor;
			block->min_args = info.min;
			block->max_args = info.any_arg ? (size_t)-1 : info.max;
		
			Tuple<Module> *scope = Tuple<Module>::allocate(1);

			(*scope)[0] = module;

			Method *result = Collector::allocate<Method>(block, scope);

			module->set_method(name, result);

			return result;
		}

		Method *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(0), wrapper, function);
		}
	};
};
