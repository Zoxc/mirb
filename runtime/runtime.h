#pragma once
#include "../globals.h"
#include "classes.h"

static inline bool rt_test(rt_value value)
{
	return value & ~RT_NIL;
}

static inline rt_value rt_alloc(size_t size)
{
	return (rt_value)malloc(size);
}

static inline rt_value rt_realloc(rt_value old, size_t size)
{
	return (rt_value)realloc((void *)old, size);
}

void rt_create(void);
void rt_destroy(void);

void rt_print(rt_value obj);

void *rt_lookup(rt_value method, rt_value obj);
rt_value rt_dump_call(rt_value obj, unsigned int argc, ...);
