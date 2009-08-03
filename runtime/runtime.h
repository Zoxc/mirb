#pragma once
#include "../globals.h"

#define RT_TYPE_SIZE 16
#define RT_TYPE_MASK (RT_TYPE_SIZE - 1)
#define RT_FLAG(i) (RT_TYPE_SIZE << (i))
#define RT_USER_FLAG(i) RT_FLAG(i + 4)

#define RT_FLAG_FIXNUM 1

#define RT_INT2FIX(imm) ((rt_value)(((rt_value)(imm) << 1) | RT_FLAG_FIXNUM))
#define RT_FIX2INT(imm) ((rt_value)((rt_value)(imm) >> 1))

typedef unsigned int rt_value;

typedef rt_value (*rt_compiled_block_t)(rt_value obj, unsigned int argc, ...);

KHASH_MAP_INIT_INT(rt_hash, rt_value);
KHASH_MAP_INIT_INT(rt_block, rt_compiled_block_t);

typedef enum {
	C_FIXNUM,
	C_TRUE,
	C_FALSE,
	C_NIL,
	C_MAIN,
	C_CLASS,
	C_MODULE,
	C_OBJECT,
	C_SYMBOL,
	C_STRING
} rt_type_t;

struct rt_common {
	unsigned int flags;
	rt_value class_of;
};

#define RT_COMMON(value) ((struct rt_common *)value)

#define RT_FALSE 0
#define RT_TRUE 2
#define RT_NIL 4
#define RT_MAIN 6

static inline rt_type_t rt_type(rt_value obj)
{
	if (obj & RT_FLAG_FIXNUM)
		return C_FIXNUM;
	else if (obj <= RT_MAIN)
	{
		switch(obj)
		{
			case RT_TRUE:
				return C_TRUE;

			case RT_FALSE:
				return C_FALSE;

			case RT_NIL:
				return C_NIL;

			case RT_MAIN:
				return C_MAIN;

			default:
				assert(0);
		}
	}
	else
		return RT_COMMON(obj)->flags & RT_TYPE_MASK;
}

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
