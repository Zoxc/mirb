#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"

rt_value rt_String;

rt_value rt_string_from_cstr(const char *str)
{
	rt_value string = rt_alloc(sizeof(struct rt_string));

	RT_COMMON(string)->flags = C_STRING;
	RT_COMMON(string)->class_of = rt_String;
	RT_STRING(string)->length = strlen(str);
	RT_STRING(string)->string =  malloc(RT_STRING(string)->length + 1);
	strcpy(RT_STRING(string)->string, str);

	return string;
}
