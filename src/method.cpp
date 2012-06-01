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

		Count::Type Count::apply(Frame &frame, State &state)
		{
			return frame.argc - state.index;
		}

		Values::Type Values::apply(Frame &frame, State &state)
		{
			return &frame.argv[state.index];
		}
		
		const intptr_t Fixnum::default_value = Mirb::Fixnum::undef;

		Fixnum::Type Fixnum::coerce(value_t value)
		{
			if(Mirb::Value::is_fixnum(value))
				return Mirb::Fixnum::to_int(value);
			else
				type_error(value, "Fixnum");
		}
		
		UInt::Type UInt::coerce(value_t value)
		{
			if(Mirb::Value::is_fixnum(value))
			{
				auto result = Mirb::Fixnum::to_int(value);

				if(result < 0)
					raise(context->standard_error, "A value above zero was expected.");
				else
					return result;
			}
			else
				type_error(value, "Fixnum above zero");
		}
		
		const value_t Value::default_value = 0;

		Value::Type Value::coerce(value_t value)
		{
			return value;
		}
	}
	
	namespace MethodGen
	{
		Method *generate_block(size_t flags prelude_unused, Module *module, Symbol *name, Arg::Info &&info, Block::executor_t executor)
		{
			Block *block = Collector::allocate_pinned<Block>(nullptr);

			module->assert_valid();

			block->executor = executor;
			block->min_args = info.min;
			block->max_args = info.any_arg ? (size_t)-1 : info.max;
		
			Tuple<Module> *scope = Tuple<Module>::allocate(1);

			(*scope)[0] = module;

			Method *result = Collector::allocate<Method>(block, scope);

			module->set_method(name, result);

			return result;
		}
	};
};
