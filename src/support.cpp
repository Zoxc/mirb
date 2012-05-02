#include "support.hpp"
#include "classes/symbol.hpp"
#include "classes/proc.hpp"
#include "classes/array.hpp"
#include "classes/string.hpp"
#include "classes/hash.hpp"
#include "runtime.hpp"

namespace Mirb
{
	namespace Support
	{
		value_t create_closure(Block *block, value_t self, Symbol *name, value_t module, size_t argc, value_t argv[])
		{
			auto &scopes = *Tuple<>::allocate(argc);
			
			for(size_t i = 0; i < argc; ++i)
				scopes[i] = argv[i];

			return auto_cast(Collector::allocate<Proc>(context->proc_class, self, name, module, block, &scopes));
		}

		value_t get_const(value_t obj, Symbol *name)
		{
			if(prelude_unlikely(obj == context->main))
				obj = context->object_class;

			return Mirb::get_const(obj, name);
		}

		value_t set_const(value_t obj, Symbol *name, value_t value)
		{
			if(prelude_unlikely(obj == context->main))
				obj = context->object_class;

			return Mirb::set_const(obj, name, value);
		}
		
		value_t get_ivar(value_t obj, Symbol *name)
		{
			return get_var(obj, name);
		}

		void set_ivar(value_t obj, Symbol *name, value_t value)
		{
			set_var(obj, name, value);
		}

		value_t interpolate(size_t argc, value_t argv[], Value::Type type)
		{
			CharArray result;

			OnStackString<1> os(result);

			for(size_t i = 0; i < argc; ++i)
			{
				value_t obj = argv[i];

				if(prelude_unlikely(Value::type(obj) != Value::String))
					obj = Mirb::call(obj, "to_s");

				if(prelude_likely(Value::type(obj) == Value::String))
					result += cast<String>(obj)->string;
			}

			if(type == Value::String)
				return result.to_string();
			else
				return symbol_pool.get(result);
		}
		
		value_t create_array(size_t argc, value_t argv[])
		{
			Array *array = Collector::allocate<Array>(context->array_class);
			
			for(size_t i = 0; i < argc; ++i)
				array->vector.push(argv[i]);

			return array;
		}
		
		value_t create_hash(size_t argc, value_t argv[])
		{
			Hash *hash = Collector::allocate<Hash>(context->hash_class);
			
			for(size_t i = 0; i < argc; i += 2)
				hash->map.set(argv[i],  argv[i + 1]);

			return hash;
		}
		
		value_t define_class(value_t obj, Symbol *name, value_t super)
		{
			if(prelude_unlikely(obj == context->main))
				obj = context->object_class;
			
			return Mirb::define_class(auto_cast(obj), name, auto_cast(super));
		}
		
		value_t define_module(value_t obj, Symbol *name)
		{
			if(prelude_unlikely(obj == context->main))
				obj = context->object_class;
			
 			return Mirb::define_module(auto_cast(obj), name);
		}
		
		void define_method(value_t obj, Symbol *name, Block *block)
		{
			if(prelude_unlikely(obj == context->main))
				obj = context->object_class;

			set_method(obj, name, block);
		}

		bool define_singleton_method(value_t obj, Symbol *name, Block *block)
		{
			Class *klass = singleton_class(obj);

			if(!klass)
				return false;

			set_method(klass, name, block);
			return true;
		}
	};
};

