#include "support.hpp"
#include "classes/symbol.hpp"
#include "classes/proc.hpp"
#include "classes/array.hpp"
#include "classes/string.hpp"
#include "classes/hash.hpp"
#include "classes/regexp.hpp"
#include "runtime.hpp"

namespace Mirb
{
	namespace Support
	{
		value_t create_closure(Block *block, value_t self, Symbol *name, Tuple<Module> *scope, size_t argc, value_t argv[])
		{
			auto &scopes = *Tuple<>::allocate(argc);
			
			for(size_t i = 0; i < argc; ++i)
				scopes[i] = argv[i];

			return auto_cast(Collector::allocate<Proc>(context->proc_class, self, name, scope, block, &scopes));
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
			
			switch(type)
			{
				case Value::Symbol:
					return result.to_string();
					break;
					
				case Value::String:
					return symbol_pool.get(result);

				case Value::Regexp:
					return Regexp::allocate(result);

				case Value::Array:
				{
					Array *array = Collector::allocate<Array>();

					Array::parse(result.raw(), result.size(), [&](const std::string &str){
						array->vector.push(CharArray(str).to_string());
					});

					return array;
				}

				default:
					mirb_debug_abort("Unknown data type");
			}
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
			{
				if(!HashAccess::set(hash, argv[i], argv[i + 1]))
					return 0;
			}

			return hash;
		}
		
		void define_method(Tuple<Module> *scope, Symbol *name, Block *block)
		{
			scope->first()->set_method(name, Collector::allocate<Method>(block, scope));
		}

		bool define_singleton_method(Tuple<Module> *scope, value_t obj, Symbol *name, Block *block)
		{
			Class *klass = singleton_class(obj);

			if(!klass)
				return false;

			klass->set_method(name, Collector::allocate<Method>(block, scope));
			return true;
		}
	};
};

