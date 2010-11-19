#include "support.hpp"
#include "../../runtime/classes/proc.hpp"
#include "../../runtime/classes/array.hpp"

namespace Mirb
{
	namespace Support
	{
		rt_value create_closure(Block *block, rt_value self, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[])
		{
			rt_value closure = (rt_value)rt_alloc(sizeof(struct rt_proc) + sizeof(rt_value *) * argc);

			RT_COMMON(closure)->flags = C_PROC;
			RT_COMMON(closure)->class_of = rt_Proc;
			RT_COMMON(closure)->vars = 0;
			RT_PROC(closure)->self = self;
			RT_PROC(closure)->closure = block;
			RT_PROC(closure)->method_name = method_name;
			RT_PROC(closure)->method_module = method_module;
			RT_PROC(closure)->scope_count = argc;

			RT_ARG_EACH_RAW(i)
			{
				RT_PROC(closure)->scopes[i] = RT_ARG(i);
			}

			return closure;
		}
		
		rt_value interpolate(size_t argc, rt_value argv[])
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
		
		rt_value create_array(size_t argc, rt_value argv[])
		{
			rt_value array = rt_alloc(sizeof(struct rt_array));

			RT_COMMON(array)->flags = C_ARRAY;
			RT_COMMON(array)->class_of = rt_Array;
			RT_COMMON(array)->vars = 0;

			RT_ARRAY(array)->data.size = argc;
			RT_ARRAY(array)->data.max = argc;
			RT_ARRAY(array)->data.array = (rt_value *)rt_alloc(argc * sizeof(rt_value));

			RT_ARG_EACH_RAW(i)
			{
				RT_ARRAY(array)->data.array[i] = RT_ARG(i);
			}

			return array;
		}
		
		rt_value define_string(const char *string)
		{
			// TODO: Make sure string is not garbage collected. Replace it with something nicer.
			return rt_string_from_cstr(string);
		}
		
		rt_value define_class(rt_value obj, rt_value name, rt_value super)
		{
			if(obj == rt_main)
				obj = rt_Object;
			
			return rt_define_class_symbol(obj, name, super);
		}
		
		rt_value define_module(rt_value obj, rt_value name)
		{
			if(obj == rt_main)
				obj = rt_Object;
			
			return rt_define_module_symbol(obj, name);
		}
		
		void define_method(rt_value obj, rt_value name, Block *block)
		{
			if(obj == rt_main)
				obj = rt_Object;

			#ifdef DEBUG
				printf("Defining method %s.%s\n", rt_string_to_cstr(rt_inspect(obj)), rt_symbol_to_cstr(name));
			#endif
			
			rt_class_set_method(obj, name, block);
		}
		
		rt_value call(Symbol *method_name, rt_value obj, rt_value block, size_t argc, rt_value argv[])
		{
			rt_value method_module;

			rt_compiled_block_t method = rt_lookup(obj, (rt_value)method_name, &method_module);

			return method((rt_value)method_name, method_module, obj, block, argc, argv);
		}

		rt_value super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
		{
			rt_value result_module;

			rt_compiled_block_t method = rt_lookup_super(method_module, (rt_value)method_name, &result_module);

			return method((rt_value)method_name, result_module, obj, block, argc, argv);
		}
	};
};

