#pragma once
#include "../globals.h"

#define RT_TYPE_SIZE 16
#define RT_TYPE_MASK (RT_TYPE_SIZE - 1)
#define RT_FLAG(i) (RT_TYPE_SIZE << (i))
#define RT_USER_FLAG(i) RT_FLAG(i + 4)

#define RT_FLAG_FIXNUM 1

typedef size_t rt_value;

typedef rt_value (__stdcall *rt_compiled_block_t)(rt_value obj, rt_value block, size_t argc, rt_value argv[]);

#define RT_ARG_INDEX(index) (argc - 1 - (index))
#define RT_ARG(index) (argv[RT_ARG_INDEX(index)])
#define RT_ARG_EACH_RAW(i) for(size_t i = 0; i < argc; i++)
#define RT_ARG_EACH(i) for(size_t i = argc - 1; i != (size_t)-1; i--)
#define RT_ARG_EACH_REV(i) for(size_t i = 0; i < argc; i++)

KHASH_MAP_INIT_INT(rt_hash, rt_value);
KHASH_MAP_INIT_INT(rt_block, rt_compiled_block_t);

typedef enum {
	C_FIXNUM,
	C_TRUE,
	C_FALSE,
	C_NIL,
	C_MAIN,
	C_CLASS,
	C_ICLASS,
	C_MODULE,
	C_OBJECT,
	C_SYMBOL,
	C_STRING,
	C_ARRAY,
	C_PROC,
	C_EXCEPTION
} rt_type_t;

struct rt_common {
	size_t flags;
	rt_value class_of;
};

#define RT_COMMON(value) ((struct rt_common *)value)

#define RT_NIL 0
#define RT_TRUE 2
#define RT_FALSE 4
#define RT_MAX RT_FALSE

static inline rt_type_t rt_type(rt_value obj)
{
	if (obj & RT_FLAG_FIXNUM)
		return C_FIXNUM;
	else if (obj <= RT_MAX)
	{
		switch(obj)
		{
			case RT_TRUE:
				return C_TRUE;

			case RT_FALSE:
				return C_FALSE;

			case RT_NIL:
				return C_NIL;

			default:
				assert(0);
		}
	}
	else
		return RT_COMMON(obj)->flags & RT_TYPE_MASK;
}

static inline bool rt_test(rt_value value)
{
	return value & ~RT_FALSE;
}

#define RT_BOOL(value) ((value)? RT_TRUE : RT_FALSE)

static inline bool rt_bool(rt_value value)
{
	return value & ~RT_FALSE;
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

rt_value rt_eval(rt_value self, const char *input, const char *filename);
void rt_print(rt_value obj);
rt_value rt_inspect(rt_value obj);

rt_compiled_block_t rt_lookup(rt_value obj, rt_value name);
rt_compiled_block_t rt_lookup_nothrow(rt_value obj, rt_value name);

#define RT_CALL_CSTR(obj, cstr, argc, argv) (rt_lookup((obj), rt_symbol_from_cstr(cstr))((obj), RT_NIL, argc, argv))
#define RT_CALL(obj, symbol, block, argc, argv) (rt_lookup((obj), (cstr))((obj), block, argc, argv))

rt_value rt_dump_call(rt_value obj, size_t argc, ...);


