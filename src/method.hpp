#pragma once
#include "common.hpp"
#include "classes/method.hpp"
#include "classes/symbol.hpp"
#include "generic/memory-pool.hpp"
#include "generic/vector.hpp"
#include "generic/simple-list.hpp"

namespace Mirb
{
	class MethodGen;
	
	namespace CodeGen
	{
		class Block;
		class ByteCodeGenerator;
	};

	namespace Tree
	{
		class Variable;
	};

	class MethodGen;

	namespace Arg
	{
		class Argument
		{
			public:
				typedef value_t type;
		};

		class Self:
			public Argument
		{
			public:
				static Tree::Variable *gen(MethodGen &g);
		};

		class Block:
			public Argument
		{
			public:
				static Tree::Variable *gen(MethodGen &g);
		};
		
		class Count:
			public Argument
		{
			public:
				typedef size_t type;

				static Tree::Variable *gen(MethodGen &g);
		};

		class Values:
			public Argument
		{
			public:
				typedef value_t *type;

				static Tree::Variable *gen(MethodGen &g);
		};

		template<typename T> void *cast_function(T *function)
		{
			value_t (*test)() mirb_unused = function;
			return (void *)function;
		}
		
		template<typename Arg1, typename T> void *cast_function(T *function)
		{
			value_t (*test)(typename Arg1::type) mirb_unused = function;
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
		
		// TODO: Use these functions elsewhere
		template<size_t length> Symbol *symbol_cast(const char (&string)[length])
		{
			return Symbol::from_literal(string);
		}
		
		static inline Symbol *symbol_cast(Symbol *symbol)
		{
			return symbol;
		}
	};
	
	class MethodGen
	{
		private:
			size_t flags;
			value_t module;
			Symbol *name;
			void *function;
			size_t arg_count;
			size_t index;
			CodeGen::Block *block;
			MemoryPool memory_pool;
			Tree::Variable **args;

			void initalize(size_t flags, value_t module, Symbol *name, void *function, size_t arg_count);
		public:
			CodeGen::ByteCodeGenerator *g;
			
			MethodGen(size_t flags, value_t module, Symbol *name, void *function, size_t arg_count);

			template<class T> void apply()
			{
				args[index++] = T::gen(*this);
			};

			Block *gen();
	};
	
	Block *generate_method(size_t flags, value_t module, Symbol *name, void *function);
	
	template<typename Arg1> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		MethodGen gen(flags, module, name, function, 1);
		gen.apply<Arg1>();
		return gen.gen();
	}
	
	template<typename Arg1, typename Arg2> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		MethodGen gen(flags, module, name, function, 2);
		gen.apply<Arg1>();
		gen.apply<Arg2>();
		return gen.gen();
	}
	
	template<typename Arg1, typename Arg2, typename Arg3> Block *generate_method(size_t flags, value_t module, Symbol *name, void *function)
	{
		MethodGen gen(flags, module, name, function, 3);
		gen.apply<Arg1>();
		gen.apply<Arg2>();
		gen.apply<Arg3>();
		return gen.gen();
	}
	
	template<class Object, value_t (Object::*function)(), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *) = &Arg::call_method<Object, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method(Method::Internal | flags, Object::class_ref, Arg::symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename Arg1, value_t (Object::*function)(typename Arg1::type arg1), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *, typename Arg1::type) = &Arg::call_method<Object, typename Arg1::type, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method<Arg1>(Method::Internal | flags, Object::class_ref, Arg::symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename Arg1, typename Arg2, value_t (Object::*function)(typename Arg1::type arg1, typename Arg2::type arg2), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *, typename Arg1::type, typename Arg2::type) = &Arg::call_method<Object, typename Arg1::type, typename Arg2::type, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method<Arg1, Arg2>(Method::Internal | flags, Object::class_ref, Arg::symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename Arg1, typename Arg2, typename Arg3, value_t (Object::*function)(typename Arg1::type arg1, typename Arg2::type arg2, typename Arg3::type arg3), typename N> Block *method(N &&name, size_t flags = 0)
	{
		value_t (*stub)(Object *, typename Arg1::type, typename Arg2::type, typename Arg3::type) = &Arg::call_method<Object, typename Arg1::type, typename Arg2::type, typename Arg3::type, function>; // TODO: Fix workaround for VC++ bug?
		return generate_method<Arg1, Arg2, Arg3>(Method::Internal | flags, Object::class_ref, Arg::symbol_cast(name), (void *)stub);
	}
	
	template<class Object, typename N, typename F> Block *singleton_method(N &&name, F function, size_t flags = 0)
	{
		return generate_method(Method::Static | Method::Singleton | flags, Object::class_ref, Arg::symbol_cast(name), Arg::cast_function(function));
	}
	
	template<class Object, typename Arg1, typename N, typename F> Block *singleton_method(N &&name, F function, size_t flags = 0)
	{
		return generate_method<Arg1>(Method::Static | Method::Singleton | flags, Object::class_ref, Arg::symbol_cast(name), Arg::cast_function<Arg1>(function));
	}
};
