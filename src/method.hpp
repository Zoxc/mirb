#pragma once
#include "common.hpp"
#include "runtime.hpp"
#include "classes/method.hpp"
#include "classes/symbol.hpp"
#include "classes/exceptions.hpp"
#include "generic/memory-pool.hpp"
#include "codegen/opcodes.hpp"
#include "macros.hpp"
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
				template<class A> struct ArgumentWrapper
				{
					typedef T Real;
				};

				static const Info info;

				typedef ResultType Type;

				static Type apply(Frame &frame, State &state)
				{
					value_t value = frame.argv[state.index++];

					return T::coerce(value);
				}
		};

		template<class T, typename ResultType> const Info ValueBase<T, ResultType>::info = {1, 1, false};
		
		class Block
		{
			public:
				template<class A> struct ArgumentWrapper
				{
					typedef Block Real;
				};

				typedef value_t Type;
				static const Info info;

				static Type apply(Frame &frame, State &state);
		};
		
		class Count
		{
			public:
				template<class A> struct ArgumentWrapper
				{
					typedef Count Real;
				};

				typedef size_t Type;
				static const Info info;

				static Type apply(Frame &frame, State &state);
		};

		class Values
		{
			public:
				template<class A> struct ArgumentWrapper
				{
					typedef Values Real;
				};

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
					if(!of_type<T>(value))
						type_error(value, "an object of type " + Type::names[Type::ToTag<T>::value]);
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
	
	template<class T> class Optional
	{
		public:
			template<class A> struct ArgumentWrapper
			{
				typedef Optional Real;
			};

			static const Arg::Info info;
			typedef typename T::template ArgumentWrapper<T>::Real Real;
			typedef typename Real::Type Type;
				
			static Type apply(Frame &frame, Arg::State &state)
			{
				if(state.index >= frame.argc)
					return Real::default_value;

				value_t result = frame.argv[state.index++];
					
				return Real::coerce(result);
			}
	};
	
	template<class T> const Arg::Info Optional<T>::info = {0, 1, false};

	template<class T> class Self
	{
		public:
			template<class A> struct ArgumentWrapper
			{
				typedef Self Real;
			};

			static const Arg::Info info;
			typedef typename T::template ArgumentWrapper<T>::Real Real;
			typedef typename Real::Type Type;
				
			static Type apply(Frame &frame, Arg::State &)
			{
				return Real::coerce(frame.obj);
			}
	};
	
	template<class T> const Arg::Info Self<T>::info = {0, 0, false};
		
	namespace MethodGen
	{
		Method *generate_block(size_t flags, Module *module, Symbol *name, Arg::Info &&info, Block::executor_t executor);
	
		#define MIRB_METHOD_TYPENAME_ARG(i, l) typename Arg##i
		#define MIRB_METHOD_ARG(i, l) Arg##i
		
		#define MIRB_METHOD_FOLD_STATEMENT(i, l) value += Arg##i::template ArgumentWrapper<Arg##i>::Real::info
		
		#define MIRB_METHOD_FOLD(i) \
			template<bool dummy MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_TYPENAME_ARG, i)> Arg::Info fold() \
			{ \
				Arg::Info value = {0, 0, false}; \
				MIRB_LOCAL_STATEMENT_LIST(MIRB_METHOD_FOLD_STATEMENT, i) \
				return value; \
			};
		
		MIRB_STATEMENT_LIST(MIRB_METHOD_FOLD, MIRB_STATEMENT_MAX)
		
		#define MIRB_METHOD_WRAPPER_STATEMENT(i, l) typename Arg##i::template ArgumentWrapper<Arg##i>::Real::Type arg##i = Arg##i::template ArgumentWrapper<Arg##i>::Real::apply(frame, state);
		#define MIRB_METHOD_WRAPPER_ARG(i, l) arg##i
		
		#define MIRB_METHOD_WRAPPER(i) \
			template<typename F, F function MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_TYPENAME_ARG, i)> value_t wrapper(Frame &frame) \
			{ \
				Arg::State state(frame, fold<false MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_ARG, i)>()); \
				\
				MIRB_LOCAL_STATEMENT_LIST(MIRB_METHOD_WRAPPER_STATEMENT, i) \
				\
				return function(MIRB_COMMA_LIST(MIRB_METHOD_WRAPPER_ARG, i)); \
			}
		
		MIRB_STATEMENT_LIST(MIRB_METHOD_WRAPPER, MIRB_STATEMENT_MAX)
	};

	#define MIRB_METHOD_ARG_TYPE(i, l) typename Arg##i::template ArgumentWrapper<Arg##i>::Real::Type

	#define MIRB_METHOD_METHOD(i) \
		template<MIRB_COMMA_AFTER_LIST(MIRB_METHOD_TYPENAME_ARG, i) value_t(*function)(MIRB_COMMA_LIST(MIRB_METHOD_ARG_TYPE, i)), typename N> Method *method(Module *module, N &&name, size_t flags = 0) \
		{ \
			return MethodGen::generate_block(flags, module, symbol_cast(std::forward<N>(name)), MethodGen::fold<false MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_ARG, i)>(), MethodGen::wrapper<value_t(*)(MIRB_COMMA_LIST(MIRB_METHOD_ARG_TYPE, i)), function MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_ARG, i)>); \
		}
		
	MIRB_STATEMENT_LIST(MIRB_METHOD_METHOD, MIRB_STATEMENT_MAX)
		
	#define MIRB_METHOD_SINGLETON(i) \
		template<MIRB_COMMA_AFTER_LIST(MIRB_METHOD_TYPENAME_ARG, i) value_t(*function)(MIRB_COMMA_LIST(MIRB_METHOD_ARG_TYPE, i)), typename N> Method *singleton_method(Object *obj, N &&name, size_t flags = 0) \
		{ \
			return MethodGen::generate_block(flags, singleton_class(obj), symbol_cast(std::forward<N>(name)), MethodGen::fold<false MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_ARG, i)>(), MethodGen::wrapper<value_t(*)(MIRB_COMMA_LIST(MIRB_METHOD_ARG_TYPE, i)), function MIRB_COMMA_BEFORE_LIST(MIRB_METHOD_ARG, i)>); \
		}

	MIRB_STATEMENT_LIST(MIRB_METHOD_SINGLETON, MIRB_STATEMENT_MAX)
};
