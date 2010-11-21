#include "support.hpp"
#include "classes/symbol.hpp"
#include "../../runtime/classes/proc.hpp"
#include "../../runtime/classes/array.hpp"
#include "../../runtime/constant.hpp"

namespace Mirb
{
	namespace Support
	{
		Value create_closure(Block *block, Value self, size_t argc, Value *argv[])
		{
			Value closure = (Value)rt_alloc(sizeof(struct rt_proc) + sizeof(Value *) * argc);

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

		Value *create_heap(size_t bytes)
		{
			return (Value *)rt_alloc(bytes);
		}
		
		Value get_const(Value obj, Symbol *name)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				std::cout << "Looking up constant " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << "\n";
			#endif

			return rt_const_get(obj, (Value)name);
		}

		void set_const(Value obj, Symbol *name, Value value)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				std::cout << "Setting constant " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << " to " << rt_string_to_cstr(rt_inspect(value)) << "\n";
			#endif

			rt_const_set(obj, (Value)name, value);
		}
		
		Value get_ivar(Value obj, Symbol *name)
		{
			#ifdef DEBUG
				std::cout << "Looking up instance variable " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << "\n";
			#endif

			return rt_object_get_var(obj, (Value)name);
		}

		void set_ivar(Value obj, Symbol *name, Value value)
		{
			#ifdef DEBUG
				std::cout << "Setting instance variable " << name->get_string() << " in " << rt_string_to_cstr(rt_inspect(obj)) << " to " << rt_string_to_cstr(rt_inspect(value)) << "\n";
			#endif

			rt_object_set_var(obj, (Value)name, value);
		}

		Value interpolate(size_t argc, Value argv[])
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
		
		Value create_array(size_t argc, Value argv[])
		{
			Value array = rt_alloc(sizeof(struct rt_array));

			RT_COMMON(array)->flags = C_ARRAY;
			RT_COMMON(array)->class_of = rt_Array;
			RT_COMMON(array)->vars = 0;

			RT_ARRAY(array)->data.size = argc;
			RT_ARRAY(array)->data.max = argc;
			RT_ARRAY(array)->data.array = (rt_value *)rt_alloc(argc * sizeof(Value));

			RT_ARG_EACH_RAW(i)
			{
				RT_ARRAY(array)->data.array[i] = RT_ARG(i);
			}

			return array;
		}
		
		Value define_string(const char *string)
		{
			// TODO: Make sure string is not garbage collected. Replace it with something nicer.
			return rt_string_from_cstr(string);
		}
		
		Value define_class(Value obj, Value name, Value super)
		{
			if(obj == rt_main)
				obj = rt_Object;
			
			return rt_define_class_symbol(obj, name, super);
		}
		
		Value define_module(Value obj, Value name)
		{
			if(obj == rt_main)
				obj = rt_Object;
			
			return rt_define_module_symbol(obj, name);
		}
		
		void define_method(Value obj, Value name, Block *block)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				printf("Defining method %s.%s\n", rt_string_to_cstr(rt_inspect(obj)), rt_symbol_to_cstr(name));
			#endif
			
			rt_class_set_method(obj, name, block);
		}
		
		Value call(Symbol *method_name, Value obj, Value block, size_t argc, Value argv[])
		{
			Value method_module;

			compiled_block_t method = (compiled_block_t)rt_lookup(obj, (Value)method_name, (rt_value *)&method_module);

			return method(method_name, method_module, obj, block, argc, argv);
		}

		Value super(Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[])
		{
			Value result_module;

			compiled_block_t method = (compiled_block_t)rt_lookup_super(method_module, (Value)method_name, (rt_value *)&result_module);

			return method(method_name, result_module, obj, block, argc, argv);
		}
	};
};

