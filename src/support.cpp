#include "support.hpp"
#include "classes/symbol.hpp"
#include "../../runtime/classes/proc.hpp"
#include "../../runtime/classes/array.hpp"
#include "../../runtime/constant.hpp"

namespace Mirb
{
	namespace Support
	{
		value_t create_closure(Block *block, value_t self, size_t argc, value_t *argv[])
		{
			value_t closure = (value_t)rt_alloc(sizeof(struct rt_proc) + sizeof(value_t *) * argc);

			RT_COMMON(closure)->flags = C_PROC;
			RT_COMMON(closure)->class_of = rt_Proc;
			RT_COMMON(closure)->vars = 0;
			RT_PROC(closure)->self = self;
			RT_PROC(closure)->closure = block;
			RT_PROC(closure)->scope_count = argc;

			RT_ARG_EACH_RAW(i)
			{
				RT_PROC(closure)->scopes[i] = (rt_value *)RT_ARG(i);
			}

			return closure;
		}

		value_t *create_heap(size_t bytes)
		{
			return (value_t *)rt_alloc(bytes);
		}
		
		value_t get_const(value_t obj, Symbol *name)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				std::cout << "Looking up constant " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << "\n";
			#endif

			return rt_const_get(obj, (value_t)name);
		}

		void set_const(value_t obj, Symbol *name, value_t value)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				std::cout << "Setting constant " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << " to " << rt_string_to_cstr(rt_inspect(value)) << "\n";
			#endif

			rt_const_set(obj, (value_t)name, value);
		}
		
		value_t get_ivar(value_t obj, Symbol *name)
		{
			#ifdef DEBUG
				std::cout << "Looking up instance variable " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << "\n";
			#endif

			return rt_object_get_var(obj, (value_t)name);
		}

		void set_ivar(value_t obj, Symbol *name, value_t value)
		{
			#ifdef DEBUG
				std::cout << "Setting instance variable " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << " to " << rt_string_to_cstr(rt_inspect(value)) << "\n";
			#endif

			rt_object_set_var(obj, (value_t)name, value);
		}

		value_t interpolate(size_t argc, value_t argv[])
		{
			size_t length = 0;

			RT_ARG_EACH(i)
			{
				if(rt_type(argv[i]) != C_STRING)
					argv[i] = rt_call(argv[i], "to_s", 0, 0);

				length += RT_STRING(argv[i])->length;
			}

			char *new_str = (char *)rt_alloc_data(length + 1);
			char *current = new_str;

			RT_ARG_EACH(i)
			{
				size_t length = RT_STRING(argv[i])->length;

				memcpy(current, RT_STRING(argv[i])->string, length);

				current += length;
			}

			*current = 0;

			return rt_string_from_raw_str(new_str, length);
		}
		
		value_t create_array(size_t argc, value_t argv[])
		{
			value_t array = rt_alloc(sizeof(struct rt_array));

			RT_COMMON(array)->flags = C_ARRAY;
			RT_COMMON(array)->class_of = rt_Array;
			RT_COMMON(array)->vars = 0;

			RT_ARRAY(array)->data.size = argc;
			RT_ARRAY(array)->data.max = argc;
			RT_ARRAY(array)->data.array = (rt_value *)rt_alloc(argc * sizeof(value_t));

			RT_ARG_EACH_RAW(i)
			{
				RT_ARRAY(array)->data.array[i] = RT_ARG(i);
			}

			return array;
		}
		
		value_t define_string(const char *string)
		{
			// TODO: Make sure string is not garbage collected. Replace it with something nicer.
			return rt_string_from_cstr(string);
		}
		
		value_t define_class(value_t obj, Symbol *name, value_t super)
		{
			if(obj == rt_main)
				obj = rt_Object;
			
			return rt_define_class_symbol(obj, (rt_value)name, super);
		}
		
		value_t define_module(value_t obj, Symbol *name)
		{
			if(obj == rt_main)
				obj = rt_Object;
			
			return rt_define_module_symbol(obj, (rt_value)name);
		}
		
		void define_method(value_t obj, Symbol *name, Block *block)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				std::cout << "Defining method " << rt_string_to_cstr(rt_inspect(obj)) << "." << name->get_string() << "\n";
			#endif
			
			rt_class_set_method(obj, (rt_value)name, block);
		}
		
		value_t call(Symbol *method_name, value_t obj, value_t block, size_t argc, value_t argv[])
		{
			value_t method_module;

			compiled_block_t method = (compiled_block_t)rt_lookup(obj, (value_t)method_name, (rt_value *)&method_module);

			return method(method_name, method_module, obj, block, argc, argv);
		}

		value_t super(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])
		{
			value_t result_module;

			compiled_block_t method = (compiled_block_t)rt_lookup_super(method_module, (value_t)method_name, (rt_value *)&result_module);

			return method(method_name, result_module, obj, block, argc, argv);
		}
	};
};

