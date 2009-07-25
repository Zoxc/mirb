#pragma once
#include "../globals.h"

#define RT_TYPE_SIZE 16
#define RT_TYPE_FLAG (RT_TYPE_SIZE - 1)
#define RT_FLAG(i) (RT_TYPE_SIZE << (i))
#define RT_USER_FLAG(i) RT_FLAG(i + 4)

typedef unsigned int rt_value;

KHASH_MAP_INIT_INT(rt_hash, rt_value);

typedef enum {
	C_FIXNUM,
	C_TRUE,
	C_FALSE,
	C_NIL,
	C_CLASS,
	C_MODULE,
	C_OBJECT,
	C_SYMBOL
} rt_type_t;

struct rt_common {
	unsigned int flags;
	rt_value class_of;
};

struct rt_class {
	struct rt_common common;
	rt_value super;
	khash_t(rt_hash) *methods;
};

#define RT_CLASS_SINGLETON RT_USER_FLAG(0)

struct rt_object {
	struct rt_common common;
};

struct rt_symbol {
	struct rt_common common;
	const char* string;
};

#define RT_COMMON(value) ((struct rt_common *)value)
#define RT_CLASS(value) ((struct rt_class *)value)
#define RT_OBJECT(value) ((struct rt_object *)value)
#define RT_SYMBOL(value) ((struct rt_symbol *)value)

extern rt_value rt_Class;
extern rt_value rt_Module;
extern rt_value rt_Object;
extern rt_value rt_Symbol;

#define RT_FLAG_FIXNUM 1
#define RT_INT2FIX(imm) ((rt_value)(((unsigned int)(imm) << 1) | RT_FLAG_FIXNUM))
#define RT_FIX2INT(imm) ((rt_value)((unsigned int)(imm) >> 1))

#define RT_FALSE 0
#define RT_TRUE 2
#define RT_NIL 4

static inline rt_type_t rt_type(rt_value obj)
{
	if (obj & RT_FLAG_FIXNUM)
		return C_FIXNUM;
	else if (obj <= 4)
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
		return RT_COMMON(obj)->flags & RT_TYPE_FLAG;
}

rt_value rt_class_create(rt_value name, rt_value super);
rt_value rt_class_create_bare(rt_value super);
rt_value rt_class_create_singleton(rt_value object, rt_value super);
