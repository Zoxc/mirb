#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "constant.h"

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name)
{
	printf("Looking up constant %s in ", rt_symbol_to_cstr(name)); rt_print(obj); printf("\n");

	return rt_const_get(obj, name);
}

void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value)
{
	printf("Setting constant %s in ", rt_symbol_to_cstr(name)); rt_print(obj); printf(" to "); rt_print(value); printf("\n");

	rt_const_set(obj, name, value);
}

rt_value __stdcall rt_support_class_create(rt_value under, rt_value name, rt_value super)
{
	rt_value obj = rt_class_create_unnamed(under, super);

	rt_class_name(obj, under, name);

	printf("Defining class %s\n", RT_STRING(rt_object_get_var(obj, rt_symbol_from_cstr("__classpath__")))->string);

	return obj;
}

rt_value __stdcall rt_support_class_create_main(rt_value name, rt_value super)
{
	return rt_support_class_create(rt_Object, name, super);
}
