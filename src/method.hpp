#pragma once
#include "common.hpp"
#include "runtime.hpp"
#include "classes/method.hpp"
#include "classes/symbol.hpp"
#include "classes/exceptions.hpp"
#include "generic/memory-pool.hpp"
#include "codegen/opcodes.hpp"
#include <algorithm>

namespace Mirb
{
	namespace CodeGen
	{
		class BasicBlock;
		class ByteCodeGenerator;
	};

	namespace Tree
	{
		class Variable;
	};

	void raise(Class *exception_class, const CharArray &message);
	void type_error(value_t value, value_t expected);

	namespace Arg
	{
		struct Info
		{
			size_t min;
			size_t max;
			bool any_arg;

			Info &operator +=(const Info &other);
		};

		struct State
		{
			size_t index;

			State(Frame &frame prelude_unused, Info &&info prelude_unused) : index(0)
			{
				mirb_debug_assert(frame.argc >= info.min);
				mirb_debug_assert(info.any_arg || frame.argc <= info.max);
			}
		};

		template<class T, typename ResultType> class ValueBase
		{
			public:
				static const Info info;

				typedef ResultType Type;

				static Type apply(Frame &frame, State &state)
				{
					value_t value = frame.argv[state.index++];

					return T::coerce(value);
				}
		};

		template<class T, typename ResultType> const Info ValueBase<T, ResultType>::info = {1, 1, false};
		
		template<class T> class Default
		{
			public:
				static const Info info;
				typedef typename T::Type Type;
				
				static Type apply(Frame &frame, State &state)
				{
					if(state.index >= frame.argc)
						return T::default_value;

					value_t result = frame.argv[state.index++];
					
					return T::coerce(result);
				}
		};
		
		template<class T> const Info Default<T>::info = {0, 1, false};

		template<class T> class Self
		{
			public:
				static const Info info;
				typedef typename T::Type Type;
				
				static Type apply(Frame &frame, State &)
				{
					return T::coerce(frame.obj);
				}
		};
		
		template<class T> const Info Self<T>::info = {0, 0, false};
		
		class Block
		{
			public:
				typedef value_t Type;
				static const Info info;

				static Type apply(Frame &frame, State &state);
		};
		
		class Count
		{
			public:
				typedef size_t Type;
				static const Info info;

				static Type apply(Frame &frame, State &state);
		};

		class Values
		{
			public:
				typedef value_t *Type;
				static const Info info;

				static Type apply(Frame &frame, State &state);
		};
		
		class Fixnum:
			public ValueBase<Fixnum, intptr_t>
		{
			public:
				static const intptr_t default_value;

				static Type coerce(value_t value);
		};
		
		class UInt:
			public ValueBase<UInt, size_t>
		{
			public:
				static Type coerce(value_t value);
		};
		
		class Value:
			public ValueBase<Value, value_t>
		{
			public:
				static const value_t default_value;

				static Type coerce(value_t value);
		};
		
		template<class T> class Class:
			public ValueBase<Class<T>, T *>
		{
			public:
				static T *const default_value;

				static T *coerce(value_t value)
				{
					if(!Mirb::Value::of_type<T>(value))
						type_error(value, "an object of type " + Mirb::Value::names[Mirb::Value::TypeTag<T>::value]);
					else
						return cast<T>(value);
				}
		};
		
		template<class T> T *const Class<T>::default_value = 0;
		
		template<Mirb::Class *Context::*field> class InstanceOf:
			public ValueBase<InstanceOf<field>, value_t>
		{
			public:
				static value_t coerce(value_t value)
				{
					type_error(value, context->*field);

					return value;
				}
		};
	};

	namespace MethodGen
	{
		Method *generate_block(size_t flags, Module *module, Symbol *name, Arg::Info &&info, Block::executor_t executor);
	
		Arg::Info fold(size_t num, ...);
		
		template<typename F, F function> value_t wrapper(Frame &frame)
		{
			return trap_exception_as_value([&]() -> value_t {
				Arg::State state(frame, fold(0));

				return function();
			});
		}
	
		template<typename F, F function, typename Arg1> value_t wrapper(Frame &frame)
		{
			return trap_exception_as_value([&]() -> value_t {
				Arg::State state(frame, fold(1, Arg1::info));

				typename Arg1::Type arg1 = Arg1::apply(frame, state);
		
				return function(arg1);
			});
		}
	
		template<typename F, F function, typename Arg1, typename Arg2> value_t wrapper(Frame &frame)
		{
			return trap_exception_as_value([&]() -> value_t {
				Arg::State state(frame, fold(2, Arg1::info, Arg2::info));

				typename Arg1::Type arg1 = Arg1::apply(frame, state);
				typename Arg2::Type arg2 = Arg2::apply(frame, state);

				return function(arg1, arg2);
			});
		}
	
		template<typename F, F function, typename Arg1, typename Arg2, typename Arg3> value_t wrapper(Frame &frame)
		{
			return trap_exception_as_value([&]() -> value_t {
				Arg::State state(frame, fold(3, Arg1::info, Arg2::info, Arg3::info));

				typename Arg1::Type arg1 = Arg1::apply(frame, state);
				typename Arg2::Type arg2 = Arg2::apply(frame, state);
				typename Arg3::Type arg3 = Arg3::apply(frame, state);

				return function(arg1, arg2, arg3);
			});
		}
	
		template<typename F, F function, typename Arg1, typename Arg2, typename Arg3, typename Arg4> value_t wrapper(Frame &frame)
		{
			return trap_exception_as_value([&]() -> value_t {
				Arg::State state(frame, fold(4, Arg1::info, Arg2::info, Arg3::info, Arg4::info));

				typename Arg1::Type arg1 = Arg1::apply(frame, state);
				typename Arg2::Type arg2 = Arg2::apply(frame, state);
				typename Arg3::Type arg3 = Arg3::apply(frame, state);
				typename Arg4::Type arg4 = Arg4::apply(frame, state);

				return function(arg1, arg2, arg3, arg4);
			});
		}

		template<typename F, F function, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> value_t wrapper(Frame &frame)
		{
			return trap_exception_as_value([&]() -> value_t {
				Arg::State state(frame, fold(5, Arg1::info, Arg2::info, Arg3::info, Arg4::info, Arg5::info));

				typename Arg1::Type arg1 = Arg1::apply(frame, state);
				typename Arg2::Type arg2 = Arg2::apply(frame, state);
				typename Arg3::Type arg3 = Arg3::apply(frame, state);
				typename Arg4::Type arg4 = Arg4::apply(frame, state);
				typename Arg5::Type arg5 = Arg5::apply(frame, state);

				return function(arg1, arg2, arg3, arg4, arg5);
			});
		}

		template<typename F, F function> Method *generate_method(size_t flags, Module *module, Symbol *name)
		{
			return generate_block(flags, module, name, fold(0), wrapper<F, function>);
		}
	
		template<typename F, F function, typename Arg1> Method *generate_method(size_t flags, Module *module, Symbol *name)
		{
			return generate_block(flags, module, name, fold(1, Arg1::info), wrapper<F, function, Arg1>);
		}
	
		template<typename F, F function, typename Arg1, typename Arg2> Method *generate_method(size_t flags, Module *module, Symbol *name)
		{
			return generate_block(flags, module, name, fold(2, Arg1::info, Arg2::info), wrapper<F, function, Arg1, Arg2>);
		}
	
		template<typename F, F function, typename Arg1, typename Arg2, typename Arg3> Method *generate_method(size_t flags, Module *module, Symbol *name)
		{
			return generate_block(flags, module, name, fold(3, Arg1::info, Arg2::info, Arg3::info), wrapper<F, function, Arg1, Arg2, Arg3>);
		}
	
		template<typename F, F function, typename Arg1, typename Arg2, typename Arg3, typename Arg4> Method *generate_method(size_t flags, Module *module, Symbol *name)
		{
			return generate_block(flags, module, name, fold(4, Arg1::info, Arg2::info, Arg3::info, Arg4::info), wrapper<F, function, Arg1, Arg2, Arg3, Arg4>);
		}

		template<typename F, F function, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> Method *generate_method(size_t flags, Module *module, Symbol *name)
		{
			return generate_block(flags, module, name, fold(5, Arg1::info, Arg2::info, Arg3::info, Arg4::info, Arg5::info), wrapper<F, function, Arg1, Arg2, Arg3, Arg4, Arg5>);
		}
	};

	template<value_t(*function)(), typename N> Method *method(Module *module, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(), function>(flags, module, symbol_cast(name));
	}
	
	template<typename Arg1, value_t(*function)(typename Arg1::Type), typename N> Method *method(Module *module, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type), function, Arg1>(flags, module, symbol_cast(name));
	}
	
	template<typename Arg1, typename Arg2, value_t(*function)(typename Arg1::Type, typename Arg2::Type), typename N> Method *method(Module *module, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type, typename Arg2::Type), function, Arg1, Arg2>(flags, module, symbol_cast(name));
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, value_t(*function)(typename Arg1::Type, typename Arg2::Type, typename Arg3::Type), typename N> Method *method(Module *module, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type, typename Arg2::Type, typename Arg3::Type), function, Arg1, Arg2, Arg3>(flags, module, symbol_cast(name));
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, value_t(*function)(typename Arg1::Type, typename Arg2::Type, typename Arg3::Type, typename Arg4::Type), typename N> Method *method(Module *module, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type, typename Arg2::Type, typename Arg3::Type, typename Arg4::Type), function, Arg1, Arg2, Arg3, Arg4>(flags, module, symbol_cast(name));
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, value_t(*function)(typename Arg1::Type, typename Arg2::Type, typename Arg3::Type, typename Arg4::Type, typename Arg5::Type), typename N> Method *method(Module *module, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type, typename Arg2::Type, typename Arg3::Type, typename Arg4::Type, typename Arg5::Type), function, Arg1, Arg2, Arg3, Arg4, Arg5>(flags, module, symbol_cast(name));
	}
	
	template<value_t(*function)(), typename N> Method *singleton_method(Object *obj, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(), function>(flags, singleton_class(obj), symbol_cast(name));
	}
	
	template<typename Arg1, value_t(*function)(typename Arg1::Type), typename N> Method *singleton_method(Object *obj, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type), function, Arg1>(flags, singleton_class(obj), symbol_cast(name));
	}

	template<typename Arg1, typename Arg2, value_t(*function)(typename Arg1::Type, typename Arg2::Type), typename N> Method *singleton_method(Object *obj, N &&name, size_t flags = 0)
	{
		return MethodGen::generate_method<value_t(*)(typename Arg1::Type, typename Arg2::Type), function, Arg1, Arg2>(flags, singleton_class(obj), symbol_cast(name));
	}
};
