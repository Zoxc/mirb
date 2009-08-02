#pragma once

#include "classes.h"

struct rt_string {
	struct rt_common common;
	char* string;
	size_t length;
};

#undef RT_STRING
#define RT_STRING(value) ((struct rt_string *)value)

extern rt_value rt_String;

rt_value rt_string_from_cstr(const char *str);

static inline const char *rt_string_to_cstr(rt_value value)
{
	return RT_STRING(value)->string;
}
