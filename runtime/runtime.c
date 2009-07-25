#include "runtime.h"
#include "symbols.h"
#include "code_heap.h"

void rt_create(void)
{
	rt_Object = rt_class_create_bare(0);
	rt_Module = rt_class_create_bare(rt_Object);
	rt_Class = rt_class_create_bare(rt_Module);

	rt_value metaclass;

	metaclass = rt_class_create_singleton(rt_Object, rt_Class);
	metaclass = rt_class_create_singleton(rt_Module, metaclass);
	metaclass = rt_class_create_singleton(rt_Class, metaclass);

	symbols_create();
	code_heap_create();

	rt_Symbol = rt_class_create(rt_Symbol_symbol, rt_Object);
	RT_COMMON(rt_Symbol_symbol)->class_of = rt_Symbol;
}

void rt_destroy(void)
{
	symbols_destroy();
	code_heap_destroy();
}

void rt_print(rt_value obj)
{
	switch(rt_type(obj))
	{
		case C_FIXNUM:
			printf("%d", RT_FIX2INT(obj));
			break;

		case C_TRUE:
			printf("true");
			break;

		case C_FALSE:
			printf("false");
			break;

		case C_NIL:
			printf("nil");
			break;

		case C_SYMBOL:
			printf("<:%s:0x%08X>", RT_SYMBOL(obj)->string, obj);
			break;

		default:
			printf("<%d:0x%08X>", rt_type(obj), obj);
	}
}

void *rt_lookup(rt_value method, rt_value obj)
{
	printf("Finding "); rt_print(obj); printf(".%s\n", RT_SYMBOL(method)->string);

	return rt_dump_call;
}

rt_value rt_dump_call(rt_value obj, unsigned int argc, ...)
{
	printf("Dumped call to "); rt_print(obj); printf(".(");

	va_list args;

	argc--;

	va_start(args, argc);

	for(int i = 0; i < argc; i++)
	{
		rt_value arg = va_arg(args, rt_value);

		rt_print(arg);

		if(i != argc - 1)
			printf(", ");
	}

	va_end(args);

	printf(") -> nil\n");

	return RT_NIL;
}
