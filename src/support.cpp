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
		value_t create_closure(Block *block, value_t self, size_t argc, value_t *argv[])
		{
			value_t **scopes = (value_t **)gc.alloc(sizeof(value_t *) * argc);
			
			MIRB_ARG_EACH_RAW(i)
			{
				scopes[i] = MIRB_ARG(i);
			}

			return auto_cast(new (gc) Proc(Proc::class_ref, self, block, argc, scopes));
		}

		value_t *create_heap(size_t bytes)
		{
			return (value_t *)gc.alloc(bytes);
		}
		
		value_t get_const(value_t obj, Symbol *name)
		{
			if(obj == main)
				obj = Object::class_ref;

			#ifdef DEBUG
				std::cout << "Looking up constant " << name->get_string() << " in " << inspect_object(obj) << "\n";
			#endif

			return Mirb::get_const(obj, name);
		}

		void set_const(value_t obj, Symbol *name, value_t value)
		{
			if(obj == main)
				obj = Object::class_ref;

			#ifdef DEBUG
				std::cout << "Setting constant " << name->get_string() << " in " << inspect_object(obj) << " to " << inspect_object(value) << "\n";
			#endif

			Mirb::set_const(obj, name, value);
		}
		
		value_t get_ivar(value_t obj, Symbol *name)
		{
			#ifdef DEBUG
				std::cout << "Looking up instance variable " << name->get_string() << " in " << inspect_object(obj) << "\n";
			#endif

			return get_var(obj, name);
		}

		void set_ivar(value_t obj, Symbol *name, value_t value)
		{
			#ifdef DEBUG
				std::cout << "Setting instance variable " << name->get_string() << " in " << inspect_object(obj) << " to " << inspect_object(value) << "\n";
			#endif

			set_var(obj, name, value);
		}

		value_t interpolate(size_t argc, value_t argv[])
		{
			CharArray result;

			MIRB_ARG_EACH(i)
			{
				value_t obj = argv[i];

				if(Value::type(obj) != Value::String)
					obj = Mirb::call(obj, "to_s");

				
				if(Value::type(obj) == Value::String)
					result += cast<String>(obj)->string;
			}

			return result.to_string();
		}
		
		value_t create_array(size_t argc, value_t argv[])
		{
			Array *array = new (gc) Array(Array::class_ref);

			MIRB_ARG_EACH(i)
				array->vector.push(MIRB_ARG(i));

			return auto_cast(array);
		}
		
		value_t define_string(const char *string)
		{
			// TODO: Make sure string is not garbage collected. Replace it with something nicer.
			return auto_cast(new (gc) String((const char_t *)string, std::strlen(string)));
		}
		
		value_t define_class(value_t obj, Symbol *name, value_t super)
		{
			if(obj == main)
				obj = Object::class_ref;
			
			return Mirb::define_class(obj, name, super);
		}
		
		value_t define_module(value_t obj, Symbol *name)
		{
			if(obj == main)
				obj = Object::class_ref;
			
			return Mirb::define_module(obj, name);
		}
		
		void define_method(value_t obj, Symbol *name, Block *block)
		{
			if(obj == main)
				obj = Object::class_ref;

			#ifdef DEBUG
				std::cout << "Defining method " << inspect_object(obj) << "." << name->get_string() << "\n";
			#endif
			
			set_method(obj, name, block);
		}
		
		value_t call(Symbol *method_name, value_t obj, value_t block, size_t argc, value_t argv[])
		{
			value_t method_module;

			compiled_block_t method = lookup(obj, method_name, &method_module);

			return method(block, method_module, obj, method_name, argc, argv);
		}

		value_t super(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])
		{
			value_t result_module;

			compiled_block_t method = lookup_super(method_module, method_name, &result_module);

			return method(block, result_module, obj, method_name, argc, argv);
		}
	};
};

