#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"

rt_value rt_String;

rt_value rt_string_from_raw_str(char *str, unsigned int length)
{
	rt_value string = rt_alloc(sizeof(struct rt_string));

	RT_COMMON(string)->flags = C_STRING;
	RT_COMMON(string)->class_of = rt_String;
	RT_STRING(string)->length = length;
	RT_STRING(string)->string = str;

	return string;
}

rt_value rt_string_from_cstr(const char *str)
{
	unsigned int length = strlen(str);
	char *new_str = malloc(length + 1);

	strcpy(new_str, str);

	return rt_string_from_raw_str(new_str, length);
}

rt_value rt_string_from_hex(rt_value value)
{
	char buffer[15];

	sprintf(buffer, "%x", value);

	return rt_string_from_cstr(buffer);
}

rt_value rt_string_inspect(rt_value obj, unsigned int argc)
{
	rt_value result = rt_string_from_cstr("\"");
	rt_string_concat(result, 1, obj);
	rt_string_concat(result, 1, rt_string_from_cstr("\""));

	return result;
}

rt_value rt_string_to_s(rt_value obj, unsigned int argc)
{
	return obj;
}

void rt_string_init(void)
{
	rt_define_method(rt_String, rt_symbol_from_cstr("inspect"), (rt_compiled_block_t)rt_string_inspect);
	rt_define_method(rt_String, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_string_to_s);
	rt_define_method(rt_String, rt_symbol_from_cstr("concat"), (rt_compiled_block_t)rt_string_concat);
	rt_define_method(rt_String, rt_symbol_from_cstr("+"), (rt_compiled_block_t)rt_string_concat);
}

rt_value rt_string_concat(rt_value obj, unsigned int argc, rt_value str)
{
	unsigned int new_length = RT_STRING(obj)->length + RT_STRING(str)->length;
	char *new_str = malloc(new_length + 1);

	memcpy(new_str, RT_STRING(obj)->string, RT_STRING(obj)->length);
	memcpy(new_str + RT_STRING(obj)->length, RT_STRING(str)->string, RT_STRING(str)->length);

	new_str[new_length] = 0;

	free(RT_STRING(obj)->string);

	RT_STRING(obj)->string = new_str;
	RT_STRING(obj)->length = new_length;

	return obj;
}
