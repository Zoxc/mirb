#include "runtime.h"
#include "code_heap.h"
#include "classes/symbol.h"
#include "classes/string.h"
#include "classes/bool.h"
#include "classes/fixnum.h"
#include "classes/proc.h"
#include "classes/module.h"
#include "classes/array.h"
#include "classes/exception.h"
#include "modules/kernel.h"
#include "../compiler/bytecode.h"
#include "../compiler/block.h"
#include "../compiler/generator/generator.h"
#include "../compiler/generator/x86.h"

void rt_create(void)
{
	rt_code_heap_create();
	rt_symbols_create();

	rt_setup_classes();

	rt_class_init();
	rt_object_init();
	rt_module_init();

	rt_kernel_init();

	rt_bool_init();
	rt_symbol_init();
	rt_string_init();
	rt_fixnum_init();
	rt_proc_init();
	rt_array_init();
	rt_exception_init();
}

void rt_destroy(void)
{
	rt_symbols_destroy();
	rt_code_heap_destroy();

	hash_destroy(rt_hash, object_var_hashes);
}

rt_value rt_eval(rt_value self, rt_value method_name, rt_value method_module, const char *input, const char *filename)
{
	struct compiler *compiler = compiler_create(input, filename);

	struct node* expression = parse_main(compiler);

	if(compiler->err_count != 0)
		return RT_NIL;

	extern rt_value get_node_name(struct node *node);

	#ifdef DEBUG
		printf("Tree: %s\n", rt_string_to_cstr(get_node_name(expression)));
	#endif

	struct block *code_block = gen_block(expression);

	struct rt_block *runtime_block = compile_block(code_block);

	compiler_destroy(compiler);

	return runtime_block->compiled(0, method_name, method_module, self, RT_NIL, 0, 0);
}

struct rt_block *rt_lookup_method(rt_value module, rt_value name, rt_value *result_module)
{
	do
	{
		struct rt_block *result = rt_class_get_method(module, name);

		if(result)
		{
            *result_module = module;
			return result;
		}

		module = RT_CLASS(module)->super;
	}
	while(module != 0);

	return 0;
}

rt_compiled_block_t rt_lookup(rt_value obj, rt_value name, rt_value *result_module)
{
	struct rt_block *result = rt_lookup_method(rt_class_of(obj), name, result_module);

	if(!result)
	{
		printf("Undefined method %s on %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
		RT_ASSERT(0);
	}

	return result->compiled;
}

rt_compiled_block_t rt_lookup_super(rt_value module, rt_value name, rt_value *result_module)
{
	struct rt_block *result = rt_lookup_method(RT_CLASS(module)->super, name, result_module);

	if(!result)
	{
		printf("No superclass method %s for %s\n", rt_string_to_cstr(rt_inspect(name)), rt_string_to_cstr(rt_inspect(module)));
		RT_ASSERT(0);
	}

	return result->compiled;
}

rt_value rt_call_block(rt_value obj, rt_value name, rt_value block, size_t argc, rt_value argv[])
{
    rt_value module;
    struct rt_block *method = rt_lookup_method(rt_class_of(obj), name, &module);

	if(!method)
	{
		printf("Undefined method %s on %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
		RT_ASSERT(0);
	}

	return method->compiled(0, name, module, obj, block, argc, argv);
}

rt_value rt_inspect(rt_value obj)
{
    rt_value dummy;
	struct rt_block *inspect = rt_lookup_method(rt_class_of(obj), rt_symbol_from_cstr("inspect"), &dummy);

	rt_value result = RT_NIL;

	if(inspect && (inspect->compiled != (rt_compiled_block_t)rt_object_inspect || rt_lookup_method(rt_class_of(obj), rt_symbol_from_cstr("to_s"), &dummy)))
		result = rt_call(obj, "inspect", 0, 0);

	if(rt_type(result) == C_STRING)
		return result;
	else
		return rt_object_to_s(0, RT_NIL, RT_NIL, obj, 0, 0, 0);
}

void rt_print(rt_value obj)
{
	rt_value string = rt_inspect(obj);

	printf("%s", rt_string_to_cstr(string));
}

