#include "support.hpp"
#include "classes/symbol.hpp"
#include "classes/proc.hpp"
#include "classes/array.hpp"
#include "classes/string.hpp"
#include "runtime.hpp"

namespace Mirb
{
	namespace Support
	{
		value_t create_closure(Block *block, value_t self, Symbol *name, value_t module, size_t argc, value_t *argv[])
		{
			value_t **scopes = (value_t **)gc.allocate(sizeof(value_t *) * argc);
			
			for(size_t i = 0; i < argc; ++i)
				scopes[i] = argv[i];

			return auto_cast(Collector::allocate<Proc>(Proc::class_ref, self, name, module, block, argc, scopes));
		}

		value_t *create_heap(size_t bytes)
		{
			return (value_t *)gc.allocate(bytes);
		}
		
		value_t get_const(value_t obj, Symbol *name)
		{
			if(mirb_unlikely(obj == main))
				obj = Object::class_ref;

			return Mirb::get_const(obj, name);
		}

		bool set_const(value_t obj, Symbol *name, value_t value)
		{
			if(mirb_unlikely(obj == main))
				obj = Object::class_ref;

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

		value_t interpolate(size_t argc, value_t argv[])
		{
			CharArray result;

			for(size_t i = 0; i < argc; ++i)
			{
				value_t obj = argv[i];

				if(mirb_unlikely(Value::type(obj) != Value::String))
					obj = Mirb::call(obj, "to_s");

				if(mirb_likely(Value::type(obj) == Value::String))
					result += cast<String>(obj)->string;
			}

			return result.to_string();
		}
		
		value_t create_array(size_t argc, value_t argv[])
		{
			Array *array = Collector::allocate<Array>(Array::class_ref);
			
			for(size_t i = 0; i < argc; ++i)
				array->vector.push(argv[i]);

			return auto_cast(array);
		}
		
		value_t create_string(const char *string)
		{
			// TODO: Make sure string is not garbage collected. Replace it with something nicer.
			return auto_cast(Collector::allocate<String>((const char_t *)string, std::strlen(string)));
		}
		
		value_t define_class(value_t obj, Symbol *name, value_t super)
		{
			if(mirb_unlikely(obj == main))
				obj = Object::class_ref;
			
			return Mirb::define_class(obj, name, super);
		}
		
		value_t define_module(value_t obj, Symbol *name)
		{
			if(mirb_unlikely(obj == main))
				obj = Object::class_ref;
			
			return Mirb::define_module(obj, name);
		}
		
		void define_method(value_t obj, Symbol *name, Block *block)
		{
			if(mirb_unlikely(obj == main))
				obj = Object::class_ref;

			set_method(obj, name, block);
		}
	};
};

