#pragma once
#include "../classes.h"

struct rt_string {
	struct rt_common common;
	char* string;
	size_t length;
};

#undef RT_STRING
#define RT_STRING(value) ((struct rt_string *)value)

extern rt_value rt_String;

rt_value rt_string_from_raw_str(char *str, size_t length);
rt_value rt_string_from_cstr(const char *str);
rt_value rt_string_from_hex(rt_value value);
rt_value rt_string_from_int(rt_value value);

static inline const char *rt_string_to_cstr(rt_value value)
{
	return RT_STRING(value)->string;
}

void rt_string_init(void);

rt_value __cdecl rt_string_concat(rt_value obj, size_t argc, rt_value str);

