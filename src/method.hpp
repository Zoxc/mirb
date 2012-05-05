#pragma once
#include "common.hpp"
#include "runtime.hpp"
#include "classes/method.hpp"
#include "classes/symbol.hpp"
#include "classes/exceptions.hpp"
#include "generic/memory-pool.hpp"
#include "codegen/opcodes.hpp"

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

	value_t raise(Class *exception_class, const CharArray &message);
	bool type_error(value_t value, value_t expected);

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
			bool error;

			State(Frame &frame prelude_unused, Info &&info prelude_unused) : index(0), error(false)
			{
				mirb_debug_assert(frame.argc >= info.min);
				mirb_debug_assert(info.any_arg || frame.argc <= info.max);
			}
		};

		class Self
		{
			public:
				typedef value_t type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};
		
		class Block
		{
			public:
				typedef value_t type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};
		
		class Count
		{
			public:
				typedef size_t type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};

		class Values
		{
			public:
				typedef value_t *type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};
		
		class UInt
		{
			public:
				typedef size_t type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};
		
		class Value
		{
			public:
				typedef value_t type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};
		
		class Default
		{
			public:
				typedef value_t type;
				static const Info info;

				static type apply(Frame &frame, State &state);
		};
		
		template<class T> class Class
		{
			public:
				typedef T *type;
				static const Info info;

				static type apply(Frame &frame, State &state)
				{
					value_t result = frame.argv[state.index++];

					if(!Mirb::Value::of_type<T>(result))
					{
						state.error = true;
						type_error(result, value_nil);
						return nullptr;
					}
					else
						return auto_cast(result);
				}
		};
		
		template<class T> const Info Class<T>::info = {1, 1, false};
		
		template<class T> class DefaultClass
		{
			public:
				typedef T *type;
				static const Info info;

				static type apply(Frame &frame, State &state)
				{
					if(state.index >= frame.argc)
						return nullptr;

					value_t result = frame.argv[state.index++];

					if(!Mirb::Value::of_type<T>(result))
					{
						state.error = true;
						type_error(result, value_nil);
						return nullptr;
					}
					else
						return auto_cast(result);
				}
		};
		
		template<class T> const Info DefaultClass<T>::info = {0, 1, false};
		
		template<class T> class SpecificClass
		{
			public:
				typedef T *type;
				static const Info info;

				static type apply(Frame &frame, State &state)
				{
					value_t result = frame.argv[state.index++];

					if(type_error(result, T::class_ref))
					{
						state.error = true;
						return nullptr;
					}
					else
						return auto_cast(result);
				}
		};

		template<class T> const Info SpecificClass<T>::info = {1, 1, false};
	};

	namespace MethodGen
	{
		Block *generate_block(size_t flags, Module *module, Symbol *name, Arg::Info &&info, Block::executor_t executor, void *function);
	
		value_t wrapper(Frame &frame);
	
		Arg::Info fold(size_t num, ...);

		template<typename Arg1> value_t wrapper(Frame &frame)
		{
			Arg::State state(frame, fold(1, Arg1::info));

			if(state.error)
				return value_raise;
		
			typename Arg1::type arg1 = Arg1::apply(frame, state);
		
			if(state.error)
				return value_raise;
		
			return ((value_t (*)(typename Arg1::type))frame.code->opcodes)(arg1);
		}
	
		template<typename Arg1, typename Arg2> value_t wrapper(Frame &frame)
		{
			Arg::State state(frame, fold(2, Arg1::info, Arg2::info));

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
			Arg::State state(frame, fold(3, Arg1::info, Arg2::info, Arg3::info));

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
			Arg::State state(frame, fold(4, Arg1::info, Arg2::info, Arg3::info, Arg4::info));

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

		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> value_t wrapper(Frame &frame)
		{
			Arg::State state(frame, fold(5, Arg1::info, Arg2::info, Arg3::info, Arg4::info, Arg5::info));

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
		
			typename Arg5::type arg5 = Arg5::apply(frame, state);

			if(state.error)
				return value_raise;
		
			return ((value_t (*)(typename Arg1::type, typename Arg2::type, typename Arg3::type, typename Arg4::type, typename Arg5::type))frame.code->opcodes)(arg1, arg2, arg3, arg4, arg5);
		}

		Block *generate_method(size_t flags, Module *module, Symbol *name, void *function);
	
		template<typename Arg1> Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(1, Arg1::info), wrapper<Arg1>, function);
		}
	
		template<typename Arg1, typename Arg2> Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(2, Arg1::info, Arg2::info), wrapper<Arg1, Arg2>, function);
		}
	
		template<typename Arg1, typename Arg2, typename Arg3> Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(3, Arg1::info, Arg2::info, Arg3::info), wrapper<Arg1, Arg2, Arg3>, function);
		}
	
		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4> Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(4, Arg1::info, Arg2::info, Arg3::info, Arg4::info), wrapper<Arg1, Arg2, Arg3, Arg4>, function);
		}

		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> Block *generate_method(size_t flags, Module *module, Symbol *name, void *function)
		{
			return generate_block(flags, module, name, fold(5, Arg1::info, Arg2::info, Arg3::info, Arg4::info, Arg5::info), wrapper<Arg1, Arg2, Arg3, Arg4, Arg5>, function);
		}
	};

	template<typename N, typename F> Block *method(Module *module, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method(flags, module, symbol_cast(name), (void *)function);
	}
	
	template<typename Arg1, typename N, typename F> Block *method(Module *module, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1>(flags, module, symbol_cast(name), (void *)function);
	}
	
	template<typename Arg1, typename Arg2, typename N, typename F> Block *method(Module *module, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1, Arg2>(flags, module, symbol_cast(name), (void *)function);
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename N, typename F> Block *method(Module *module, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1, Arg2, Arg3>(flags, module, symbol_cast(name), (void *)function);
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename N, typename F> Block *method(Module *module, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1, Arg2, Arg3, Arg4>(flags, module, symbol_cast(name), (void *)function);
	}
	
	template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename N, typename F> Block *method(Module *module, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1, Arg2, Arg3, Arg4, Arg5>(flags, module, symbol_cast(name), (void *)function);
	}
	
	template<typename N, typename F> Block *singleton_method(Object *obj, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method(flags, singleton_class(obj), symbol_cast(name),  (void *)function);
	}
	
	template<typename Arg1, typename N, typename F> Block *singleton_method(Object *obj, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1>(flags, singleton_class(obj), symbol_cast(name), (void *)function);
	}

	template<typename Arg1, typename Arg2, typename N, typename F> Block *singleton_method(Object *obj, N &&name, F function, size_t flags = 0)
	{
		return MethodGen::generate_method<Arg1, Arg2>(flags, singleton_class(obj), symbol_cast(name), (void *)function);
	}
};
