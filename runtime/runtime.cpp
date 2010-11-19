#include "../src/compiler.hpp"

#ifdef DEBUG
	#include "../src/tree/printer.hpp"
#endif


using namespace Mirb;

#include "runtime.hpp"
#include "code_heap.hpp"
#include "classes/symbol.hpp"
#include "classes/string.hpp"
#include "classes/bool.hpp"
#include "classes/fixnum.hpp"
#include "classes/proc.hpp"
#include "classes/module.hpp"
#include "classes/array.hpp"
#include "classes/exception.hpp"
#include "modules/kernel.hpp"

void rt_create(void)
{
	rt_code_heap_create();

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
	rt_code_heap_destroy();

	rt_destroy_classes();
}

rt_value rt_eval(rt_value self, rt_value method_name, rt_value method_module, const char *input, const char *filename)
{
	MemoryPool memory_pool;
	Parser parser(symbol_pool, memory_pool);
	
	parser.filename = filename;
	parser.load((const char_t *)input, strlen(input));
	
 	Tree::Fragment fragment(0, Tree::Chunk::main_size);
	Tree::Scope *scope = parser.parse_main(&fragment);
	
	if(!parser.messages.empty())
	{
		for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
			std::cout << i().format() << "\n";

		return RT_NIL;
	}
	
	#ifdef DEBUG
		DebugPrinter printer;
		
		std::cout << "Parsing done.\n-----\n";
		std::cout << printer.print_node(scope->group);
		std::cout << "\n-----\n";
	#endif
	
	Block *block = Compiler::compile(scope, memory_pool);

	rt_value result = block->compiled(method_name, method_module, self, RT_NIL, 0, 0);

	block = 0; // Make sure runtime_block stays on stack*/

	return result;
}

Mirb::Block *rt_lookup_method(rt_value module, rt_value name, rt_value *result_module)
{
	do
	{
		Mirb::Block *result = rt_class_get_method(module, name);

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
	Mirb::Block *result = rt_lookup_method(rt_class_of(obj), name, result_module);

	if(!result)
	{
		printf("Undefined method %s on %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
		RT_ASSERT(0);
	}

	return result->compiled;
}

rt_compiled_block_t rt_lookup_super(rt_value module, rt_value name, rt_value *result_module)
{
	Mirb::Block *result = rt_lookup_method(RT_CLASS(module)->super, name, result_module);

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
	Mirb::Block *method = rt_lookup_method(rt_class_of(obj), name, &module);

	if(!method)
	{
		printf("Undefined method %s on %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
		RT_ASSERT(0);
	}
	
	return method->compiled(name, module, obj, block, argc, argv);
}

rt_value rt_inspect(rt_value obj)
{
	rt_value dummy;
	Mirb::Block *inspect = rt_lookup_method(rt_class_of(obj), rt_symbol_from_cstr("inspect"), &dummy);

	rt_value result = RT_NIL;

	if(inspect && (inspect->compiled != (rt_compiled_block_t)rt_object_inspect || rt_lookup_method(rt_class_of(obj), rt_symbol_from_cstr("to_s"), &dummy)))
		result = rt_call(obj, "inspect", 0, 0);

	if(rt_type(result) == C_STRING)
		return result;
	else
		return rt_object_to_s(RT_NIL, RT_NIL, obj, 0, 0, 0);
}

void rt_print(rt_value obj)
{
	rt_value string = rt_inspect(obj);

	printf("%s", rt_string_to_cstr(string));
}
