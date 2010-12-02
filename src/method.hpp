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

	namespace Arg
	{
		class Argument
		{
			public:
				void apply(MethodGen &gen) const;
		};

		class Self:
			public Argument
		{
		};

		class Block:
			public Argument
		{
		};
		
		class Count:
			public Argument
		{
		};

		class Values:
			public Argument
		{
		};

		extern Self self;
		extern Block block;
		extern Count count;
		extern Values values;

		template<typename T> void *void_cast(T *function)
		{
			return (void *)function;
		}
	};

	class MethodGen
	{
		private:
			MemoryPool memory_pool;

			void initalize(size_t flags, value_t module, Symbol *name, void *function);
		public:
			Vector<const Arg::Argument *, MemoryPool> arguments;

			MethodGen(size_t flags, value_t module, Symbol *name, void *function) : arguments(memory_pool)
			{
				initalize(flags, module, name, function);
			}

			template<size_t length> MethodGen(size_t flags, value_t module, const char (&name)[length], void *function) : arguments(memory_pool)
			{
				initalize(flags, module, Symbol::from_literal(name), function);
			}

			Block *gen();
	};
	
	template<class Object, typename N, typename F> Block *method(N &&name, F function, size_t flags = 0)
	{
		MethodGen gen(Method::Internal | flags, Object::class_ref, std::forward<N>(name), Arg::void_cast(function));
		return gen.gen();
	}
	
	template<class Object, typename N, typename F> Block *method(N &&name, F function, const Arg::Argument &arg1, size_t flags = 0)
	{
		MethodGen gen(Method::Internal | flags, Object::class_ref, std::forward<N>(name), Arg::void_cast(function));
		arg1.apply(gen);
		return gen.gen();
	}
	
	template<class Object, typename N, typename F> Block *singleton_method(N &&name, F function, size_t flags = 0)
	{
		MethodGen gen(Method::Static | Method::Singleton | flags, Object::class_ref, std::forward<N>(name), Arg::void_cast(function));
		return gen.gen();
	}
	
	template<class Object, typename N, typename F> Block *singleton_method(N &&name, F function, const Arg::Argument &arg1, size_t flags = 0)
	{
		MethodGen gen(Method::Static | Method::Singleton| flags, Object::class_ref, std::forward<N>(name), Arg::void_cast(function));
		arg1.apply(gen);
		return gen.gen();
	}
};
