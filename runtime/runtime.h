#pragma once
#include "../globals.h"
#include "gc.h"

#define RT_TYPE_SIZE 16
#define RT_TYPE_MASK (RT_TYPE_SIZE - 1)
#define RT_FLAG(i) (RT_TYPE_SIZE << (i))
#define RT_USER_FLAG(i) RT_FLAG(i + 4)

#define RT_FLAG_FIXNUM 1

typedef size_t rt_value;

typedef rt_value (__stdcall __regparm(3) *rt_compiled_block_t)(rt_value **_scopes, rt_value _method_name, rt_value _method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);

#define rt_compiled_block(name) rt_value __stdcall __regparm(3) name(rt_value **_scopes, rt_value _method_name, rt_value _method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])

#define RT_ARG_INDEX(index) (argc - 1 - (index))
#define RT_ARG(index) (argv[RT_ARG_INDEX(index)])
#define RT_ARG_EACH_RAW(i) for(size_t i = 0; i < argc; i++)
#define RT_ARG_EACH(i) for(size_t i = argc - 1; i != (size_t)-1; i--)
#define RT_ARG_EACH_REV(i) for(size_t i = 0; i < argc; i++)

/*
 * Memory allocator functions
 */

static inline rt_value rt_alloc(size_t size)
{
	#ifdef VALGRIND
		return (rt_value)malloc(size);
	#else
		return (rt_value)GC_MALLOC(size);
	#endif
}

static inline rt_value rt_alloc_root(size_t size)
{
	#ifdef VALGRIND
		return (rt_value)malloc(size);
	#else
		return (rt_value)GC_MALLOC_UNCOLLECTABLE(size);
	#endif
}

static inline rt_value rt_alloc_data(size_t size)
{
	#ifdef VALGRIND
		return (rt_value)malloc(size);
	#else
		return (rt_value)GC_MALLOC_ATOMIC(size);
	#endif
}

static inline rt_value rt_realloc(rt_value old, size_t size)
{
	#ifdef VALGRIND
		return (rt_value)realloc((void *)old, size);
	#else
		return old ? (rt_value)GC_REALLOC((void *)old, size) : rt_alloc(size);
	#endif
}

/*
 * Generic data structures
 */

#define rt_runtime_malloc(size) rt_alloc(size)
#define rt_runtime_realloc(old, old_size, new_size) rt_realloc((rt_value)(old), new_size)
#define rt_runtime_free(obj)

#define VEC_RUNTIME(type, name) \
	VEC_INIT(type, name, , , , , rt_runtime_malloc, rt_runtime_realloc, rt_runtime_free)

#define HASH_RUNTIME(name, key_t, val_t)								\
	HASH_INIT(name, key_t, val_t, 1, hash_int_hash_func, hash_int_hash_equal, , , , rt_runtime_malloc, rt_runtime_malloc, rt_runtime_realloc, rt_runtime_free)

#define HASH_RUNTIME_ROOT(name, key_t, val_t)								\
	HASH_INIT(name, key_t, val_t, 1, hash_int_hash_func, hash_int_hash_equal, , , , rt_alloc_root, rt_runtime_malloc, rt_runtime_realloc, rt_runtime_free)

#define HASH_RUNTIME_STR(name, val_t)								\
	HASH_INIT(name, const char *, val_t, 1, hash_str_hash_func, hash_str_hash_equal, , , , rt_runtime_malloc, rt_runtime_malloc, rt_runtime_realloc, rt_runtime_free)

#define HASH_RUNTIME_ROOT_STR(name, val_t)								\
	HASH_INIT(name, const char *, val_t, 1, hash_str_hash_func, hash_str_hash_equal, , , , rt_alloc_root, rt_runtime_malloc, rt_runtime_realloc, rt_runtime_free)

/*
 * Structure for code blocks
 */

struct rt_block;

VEC_RUNTIME(struct rt_block *, rt_blocks);

struct rt_block {
	rt_compiled_block_t compiled; // A pointer to a compiled function.
	rt_value name; // The name of this block.
	vec_t(rt_blocks) blocks; // A list of child blocks so the GC won't free them.
};


enum rt_type {
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
};

struct rt_common {
	size_t flags;
	rt_value class_of;
};

#define RT_COMMON(value) ((struct rt_common *)value)

#define RT_NIL 0
#define RT_TRUE 2
#define RT_FALSE 4
#define RT_MAX RT_FALSE

static inline enum rt_type rt_type(rt_value obj)
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
				RT_ASSERT(0);
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

HASH_RUNTIME(rt_hash, rt_value, rt_value);
HASH_RUNTIME(rt_methods, rt_value, struct rt_block *);

void rt_create(void);
void rt_destroy(void);

rt_value rt_eval(rt_value self, rt_value method_name, rt_value method_module, const char *input, const char *filename);
void rt_print(rt_value obj);
rt_value rt_inspect(rt_value obj);

rt_compiled_block_t __cdecl rt_lookup(rt_value obj, rt_value name, rt_value *result_module);
struct rt_block *rt_lookup_method(rt_value module, rt_value name, rt_value *result_module);
rt_compiled_block_t __cdecl rt_lookup_super(rt_value module, rt_value name, rt_value *result_module);

rt_value rt_call_block(rt_value obj, rt_value name, rt_value block, size_t argc, rt_value argv[]);

extern rt_value rt_symbol_from_cstr(const char *name);

static inline rt_value rt_call(rt_value obj, char *name, size_t argc, rt_value argv[])
{
	return rt_call_block(obj, rt_symbol_from_cstr(name), RT_NIL, argc, argv);
}

