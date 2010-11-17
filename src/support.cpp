#include "support.hpp"
#include "../../runtime/classes/proc.hpp"

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

		rt_value call(rt_value method_name, rt_value obj, rt_value block, size_t argc, rt_value argv[])
		{
			rt_value method_module;

			rt_compiled_block_t method = rt_lookup(obj, method_name, &method_module);

			return method(method_name, method_module, obj, block, argc, argv);
		}

		rt_value super(rt_value method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
		{
			rt_value result_module;

			rt_compiled_block_t method = rt_lookup_super(method_module, method_name, &result_module);

			return method(method_name, result_module, obj, block, argc, argv);
		}
	};
};

