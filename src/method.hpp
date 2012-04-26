#pragma once
#include "common.hpp"
#include "classes/method.hpp"
#include "classes/symbol.hpp"
#include "classes/exceptions.hpp"
#include "generic/memory-pool.hpp"
#include "codegen/opcodes.hpp"

namespace Mirb
{
	class MethodGen;
	
	namespace CodeGen
	{
		class Block;
		class BasicBlock;
		class ByteCodeGenerator;
	};

	namespace Tree
	{
		class Variable;
	};

	class MethodGen;

	value_t raise(Class *exception_class, const CharArray &message);
	bool type_error(value_t value, value_t expected);

	namespace Arg
	{
		struct State
		{
			size_t index;
			bool error;

			State(Frame &frame, size_t params, size_t any) : index(0), error(false)
			{
				if(!any && params != frame.argc)
				{
					Mirb::raise(context->argument_error, "Wrong number of arguments " + CharArray::uint(params) + " expected, " + CharArray::uint(frame.argc) + " provided");
					error = true;
				}
			}
		};

		class Self
		{
			public:
				typedef value_t type;
				static const size_t consumes = 0;
				static const bool any_arg = false;

				static type apply(Frame &frame, State &state);
		};

		class Block
		{
			public:
				typedef value_t type;
				static const size_t consumes = 0;
				static const bool any_arg = false;

				static type apply(Frame &frame, State &state);
		};
		
		class Count
		{
			public:
				typedef size_t type;
				static const size_t consumes = 0;
				static const bool any_arg = true;

				static type apply(Frame &frame, State &state);
		};

		class Values
		{
			public:
				typedef value_t *type;
				static const size_t consumes = 0;
				static const bool any_arg = false;

				static type apply(Frame &frame, State &state);
		};
		
		class Value
		{
			public:
				typedef value_t type;
				static const size_t consumes = 1;
				static const bool any_arg = false;

				static type apply(Frame &frame, State &state);
		};
		
		template<class T> class Class
		{
			public:
				typedef value_t type;
				static const size_t consumes = 1;
				static const bool any_arg = false;

				static type apply(Frame &frame, State &state)
				{
					value_t result = frame.argv[state.index++];

					if(!Mirb::Value::of_type<T>(result))
					{
						state.error = true;
						type_error(result, value_nil);
						return value_raise;
					}
					else
						return result;
				}
		};
		
		template<class T> class SpecificClass
		{
			public:
				typedef value_t type;
				static const size_t consumes = 1;
				static const bool any_arg = false;

				static type apply(Frame &frame, State &state)
				{
					value_t result = frame.argv[state.index++];

					if(type_error(result, T::class_ref))
					{
						state.error = true;
						return value_raise;
					}
					else
						return result;
				}
		};

		template<typename T> void *cast_function(T *function)
		{
			value_t (*test)() prelude_unused = function;
			return (void *)function;
		}
		
		template<typename Arg1, typename T> void *cast_function(T *function)
		{
			value_t (*test)(typename Arg1::type) prelude_unused = function;
			return (void *)function;
		}
		
		template<typename Arg1, typename Arg2, typename T> void *cast_function(T *function)
		{
			value_t (*test)(typename Arg1::type, typename Arg2::type) prelude_unused = function;
			return (void *)function;
		}
		
		template<typename Arg1, typename Arg2, typename Arg3, typename T> void *cast_function(T *function)
		{
			value_t (*test)(typename Arg1::type, typename Arg2::type, typename Arg3::type) prelude_unused = function;
			return (void *)function;
		}
		
		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename T> void *cast_function(T *function)
		{
			value_t (*test)(typename Arg1::type, typename Arg2::type, typename Arg3::type, typename Arg4::type) prelude_unused = function;
			return (void *)function;
		}
		
		template<typename Object, value_t (Object::*function)()> value_t call_method(Object *obj)
		{
			return (obj->*function)();
		}
		
		template<typename Object, typename Arg1, value_t (Object::*function)(Arg1 arg1)> value_t call_method(Object *obj, Arg1 arg1)
		{
			return (obj->*function)(arg1);
		}
		
		template<typename Object, typename Arg1, typename Arg2, value_t (Object::*function)(Arg1 arg1, Arg2 arg2)> value_t call_method(Object *obj, Arg1 arg1, Arg2 arg2)
		{
			return (obj->*function)(arg1, arg2);
		}
		
		template<typename Object, typename Arg1, typename Arg2, typename Arg3, value_t (Object::*function)(Arg1 arg1, Arg2 arg2, Arg3 arg3)> value_t call_method(Object *obj, Arg1 arg1, Arg2 arg2, Arg3 arg3)
		{
			return (obj->*function)(arg1, arg2, arg3);
		}
	};
	
	Block *generate_block(size_t flags, value_t module, Symbol *name, Block::executor_t executor, void *function);
	
	value_t wrapper(Frame &frame);
	
	template<typename Arg1> value_t wrapper(Frame &frame)
	{
		Arg::State state(frame, Arg1::consumes, Arg1::any_arg);

		if(state.error)
			return value_raise;
		
		typename Arg1::type arg1 = Arg1::apply(frame, state);
		
		if(state.error)
			return value_raise;
		
		return ((value_t (*)(typename Arg1::type))frame.code->opcodes)(arg1);
	}
	
	template<typename Arg1, typename Arg2> value_t wrapper(Frame &frame)
	{
		Arg::State state(frame, Arg1::consumes + Arg2::consumes, Arg1::any_arg || Arg2::any_arg);

		if(state.error)
			return value_raise;
		
		typename Arg1::type arg1 = Arg1::apply(frame, state);

		if(state.error)
			return value_raise;
		
		typename Arg2::type arg2 = Arg2::apply(frame, state);

		if(state.error)
			return value_raise;
		
		return ((value_t (*)(typename Arg1::type, typename Arg2::type))frame.code->opcodes)(arg1, arg2);
	}
	
	template<typename Arg1, typename Arg2, typename Arg3> value_t wrapper(Frame &frame)
	{
		Arg::State state(frame, Arg1::consumes + Arg2::consumes + Arg3::consumes, Arg1::any_arg || Arg2::any_arg || Arg3::any_arg);

		if(state.error)
			return value_raise;
		
		typename Arg1::type arg1 = Arg1::apply(frame, state);

		if(state.error)
			return value_raise;
		
		typename Arg2::type arg2 = Arg2::apply(frame, state);

		if(state.error)
			return value_raise;
		
		typename Arg3::type arg3 = Arg3::apply(frame, state);

		if(state.error)
			return value_raise;
		
		return ((value_t (*)(typename Arg1::type, typename Arg2::type, typename Arg3::type))frame.code->opcodes)(arg1, arg2, arg3);
	}

	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4> value_t wrapper(Frame &frame)
	{
		Arg::State state(frame, Arg1::consumes + Arg2::consumes + Arg3::consumes + Arg4::consumes, Arg1::any_arg || Arg2::any_arg || Arg3::any_arg || Arg4::any_arg);

		typename Arg1::type arg1 = Arg1::apply(frame, state);

		if(state.error)
			return value_raise;
		
		typename Arg2::type arg2 = Arg2::apply(frame, state);

		if(state.error)
			return value_raise;
		
		typename Arg3::type arg3 = Arg3::apply(frame, state);

		if(state.error)
			return value_raise;
		
		typename Arg4::type arg4 = Arg4::apply(frame, state);

		if(state.error)
			return value_raise;
		
		return ((value_t (*)(typename Arg1::type, typename Arg2::type, typename Arg3::type, typename Arg4::type))frame.code->opcodes)(arg1, arg2, arg3, arg4);
	}

	Block *generate_method(size_t flags, value_t module, Symbol *name, void *function);
	
	template<typename Arg1> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		return generate_block(flags, module, name, wrapper<Arg1>, function);
	}
	
	template<typename Arg1, typename Arg2> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		return generate_block(flags, module, name, wrapper<Arg1, Arg2>, function);
	}
	
	template<typename Arg1, typename Arg2, typename Arg3> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		return generate_block(flags, module, name, wrapper<Arg1, Arg2, Arg3>, function);
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		return generate_block(flags, module, name, wrapper<Arg1, Arg2, Arg3, Arg4>, function);
	}
	/*
	template<class Object, value_t (Object::*function)(), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *) = &Arg::call_method<Object, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method(Method::Internal | flags, context->object_class, symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename Arg1, value_t (Object::*function)(typename Arg1::type arg1), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *, typename Arg1::type) = &Arg::call_method<Object, typename Arg1::type, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method<Arg1>(Method::Internal | flags, context->object_class, symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename Arg1, typename Arg2, value_t (Object::*function)(typename Arg1::type arg1, typename Arg2::type arg2), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *, typename Arg1::type, typename Arg2::type) = &Arg::call_method<Object, typename Arg1::type, typename Arg2::type, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method<Arg1, Arg2>(Method::Internal | flags, context->object_class, symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename Arg1, typename Arg2, typename Arg3, value_t (Object::*function)(typename Arg1::type arg1, typename Arg2::type arg2, typename Arg3::type arg3), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *, typename Arg1::type, typename Arg2::type, typename Arg3::type) = &Arg::call_method<Object, typename Arg1::type, typename Arg2::type, typename Arg3::type, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method<Arg1, Arg2, Arg3>(Method::Internal | flags, context->object_class, symbol_cast(name), (void *)stub);
	}
	*/
	template<typename N, typename F> Block *static_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method(Method::Static | flags, module, symbol_cast(name), Arg::cast_function(function));
	}
	
	template<typename Arg1, typename N, typename F> Block *static_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1>(Method::Static | flags, module, symbol_cast(name), Arg::cast_function<Arg1>(function));
	}
	
	template<typename Arg1, typename Arg2, typename N, typename F> Block *static_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1, Arg2>(Method::Static | flags, module, symbol_cast(name), Arg::cast_function<Arg1, Arg2>(function));
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename N, typename F> Block *static_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1, Arg2, Arg3>(Method::Static | flags, module, symbol_cast(name), Arg::cast_function<Arg1, Arg2, Arg3>(function));
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename N, typename F> Block *static_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1, Arg2, Arg3, Arg4>(Method::Static | flags, module, symbol_cast(name), Arg::cast_function<Arg1, Arg2, Arg3, Arg4>(function));
	}
	
	template<typename N, typename F> Block *singleton_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method(Method::Static | Method::Singleton | flags, module, symbol_cast(name), Arg::cast_function(function));
	}
	
	template<typename Arg1, typename N, typename F> Block *singleton_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1>(Method::Static | Method::Singleton | flags, module, symbol_cast(name), Arg::cast_function<Arg1>(function));
	}

	template<typename Arg1, typename Arg2, typename N, typename F> Block *singleton_method(value_t module, N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1, Arg2>(Method::Static | Method::Singleton | flags, module, symbol_cast(name), Arg::cast_function<Arg1, Arg2>(function));
	}
};
