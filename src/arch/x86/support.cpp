#include "support.hpp"

rt_value __fastcall rt_support_call(rt_value method_name, rt_value dummy, rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value method_module;

	rt_compiled_block_t method = rt_lookup(obj, method_name, &method_module);

	return method(method_name, method_module, obj, block, argc, argv);
}

rt_value __fastcall rt_support_super(rt_value method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value result_module;

	rt_compiled_block_t method = rt_lookup_super(method_module, method_name, &result_module);

	return method(method_name, result_module, obj, block, argc, argv);
}
