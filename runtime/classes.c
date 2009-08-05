#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "constant.h"

rt_value rt_main;
rt_value rt_Class;
rt_value rt_Module;
rt_value rt_Object;

khash_t(rt_hash) *object_var_hashes;

static inline bool rt_object_has_vars(rt_value obj)
{
	if (obj & RT_FLAG_FIXNUM)
		return false;
	else if (obj <= RT_NIL)
		return false;
	else
		switch(RT_COMMON(obj)->flags & RT_TYPE_MASK)
		{
			case C_CLASS:
			case C_MODULE:
			case C_OBJECT:
				return true;

			default:
				return false;
		}
}

khash_t(rt_hash) *rt_object_get_vars(rt_value obj)
{
	if (rt_object_has_vars(obj))
	{
		khash_t(rt_hash) *result = RT_OBJECT(obj)->vars;

		if(!result)
		{
			result = kh_init(rt_hash);
			RT_OBJECT(obj)->vars = result;
		}

		return result;
	}
	else
	{
		khiter_t k = kh_get(rt_hash, object_var_hashes, obj);

		if (k != kh_end(object_var_hashes))
			return (khash_t(rt_hash) *)kh_value(object_var_hashes, k);

		int ret;

		k = kh_put(rt_hash, object_var_hashes, obj, &ret);

		assert(ret);

		khash_t(rt_hash) *hash = kh_init(rt_hash);

		kh_value(object_var_hashes, k) = (rt_value)hash;

		return hash;
	}
}

rt_value rt_class_create_bare(rt_value super)
{
	rt_value c = rt_alloc(sizeof(struct rt_class));

	RT_COMMON(c)->flags = C_CLASS;
	RT_COMMON(c)->class_of = rt_Class;
	RT_OBJECT(c)->vars = kh_init(rt_hash);
	RT_CLASS(c)->super = super;
	RT_CLASS(c)->methods = kh_init(rt_block);

	return c;
}

rt_value rt_class_create_singleton(rt_value object, rt_value super)
{
	rt_value singleton = rt_class_create_bare(super);

	RT_COMMON(singleton)->flags |= RT_CLASS_SINGLETON;
	RT_COMMON(object)->class_of = singleton;

	rt_object_set_var(singleton, rt_symbol_from_cstr("__attached__"), object);

	if(rt_type(object) == C_CLASS)
	{
		RT_COMMON(singleton)->class_of = singleton;

		//if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
		//	RT_CLASS(singleton)->super = rt_class_real(RT_CLASS(object)->super)->class_of;
	}

	return singleton;
}

rt_value rt_singleton_class(rt_value object)
{
	rt_value c = rt_class_of(object);

	if(RT_COMMON(c)->flags & RT_CLASS_SINGLETON)
		return c;

	return rt_class_create_singleton(object, c);
}

rt_value rt_class_create_unnamed(rt_value super)
{
	rt_value obj = rt_class_create_bare(super);
	rt_class_create_singleton(obj, RT_COMMON(super)->class_of);

	return obj;
}

void rt_class_name(rt_value obj, rt_value under, rt_value name)
{
	rt_value under_path = rt_object_get_var(under, rt_symbol_from_cstr("__classpath__"));

	char *new_path = malloc(RT_STRING(under)->length + strlen(rt_symbol_to_cstr(name)) + 3);

	if (under == rt_Object)
	{
		strcpy(new_path, rt_symbol_to_cstr(name));
	}
	else
	{
		strcpy(new_path, rt_string_to_cstr(under_path));
		strcat(new_path, "::");
		strcat(new_path, rt_symbol_to_cstr(name));
	}

	rt_object_set_var(obj, rt_symbol_from_cstr("__classname__"), rt_string_from_cstr(rt_symbol_to_cstr(name)));
	rt_object_set_var(obj, rt_symbol_from_cstr("__classpath__"), rt_string_from_cstr(new_path));

	rt_const_set(under, name, obj);
}

rt_value rt_define_class(rt_value under, rt_value name, rt_value super)
{
	rt_value obj = rt_class_create_unnamed(super);

	rt_class_name(obj, under, name);

	printf("Defining class %s\n", rt_string_to_cstr(rt_to_s(obj)));

	return obj;
}

void rt_define_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	rt_class_set_method(obj, name, block);

	printf("Defining method %s.%s\n", rt_string_to_cstr(rt_to_s(obj)), rt_symbol_to_cstr(name));
}

void rt_define_singleton_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	rt_define_method(rt_singleton_class(obj), name, block);
}

/*
	Object
*/

rt_value rt_object_allocate(rt_value obj, unsigned int argc)
{
	rt_value result = rt_alloc(sizeof(struct rt_object));

	RT_COMMON(result)->flags = C_OBJECT;
	RT_COMMON(result)->class_of = obj;
	RT_OBJECT(result)->vars = 0;

	return result;
}

rt_value rt_object_to_s(rt_value obj, unsigned int argc)
{
	rt_value c = rt_real_class_of(obj);
	rt_value name = rt_object_get_var(c, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
	{
		rt_value result = rt_string_from_cstr("#<");

		rt_string_concat(result, 1, name);
		rt_string_concat(result, 1, rt_string_from_cstr(":0x"));
		rt_string_concat(result, 1, rt_string_from_hex(obj));
		rt_string_concat(result, 1, rt_string_from_cstr(">"));

		return result;
	}
	else
	{
		rt_value result = rt_string_from_cstr("#<0x");

		rt_string_concat(result, 1, rt_string_from_hex(obj));
		rt_string_concat(result, 1, rt_string_from_cstr(">"));

		return result;
	}
}

/*
	Class
*/

rt_value rt_class_to_s(rt_value obj, unsigned int argc)
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;
	else if(RT_COMMON(obj)->flags & RT_CLASS_SINGLETON)
	{
		rt_value real = rt_object_get_var(obj, rt_symbol_from_cstr("__attached__"));

		rt_compiled_block_t to_s = rt_lookup(real, rt_symbol_from_cstr("to_s"));

		if(to_s)
			real = to_s(real, 0);
		else
			real = rt_class_to_s(obj, 0);

		rt_value result = rt_string_from_cstr("#<Class:");
		rt_string_concat(result, 1, real);
		rt_string_concat(result, 1, rt_string_from_cstr(">"));

		return result;
	}
	else
	{
		rt_value result = rt_string_from_cstr("#<Class:0x");

		rt_string_concat(result, 1, rt_string_from_hex(obj));
		rt_string_concat(result, 1, rt_string_from_cstr(">"));

		return result;
	}
}

/*
	main
*/

rt_value rt_main_to_s(rt_value obj, unsigned int argc)
{
	return rt_string_from_cstr("main");
}

void rt_setup_classes(void)
{
	object_var_hashes = kh_init(rt_hash);

	rt_Object = rt_class_create_bare(0);
	rt_Module = rt_class_create_bare(rt_Object);
	rt_Class = rt_class_create_bare(rt_Module);

	rt_value metaclass;

	metaclass = rt_class_create_singleton(rt_Object, rt_Class);
	metaclass = rt_class_create_singleton(rt_Module, metaclass);
	metaclass = rt_class_create_singleton(rt_Class, metaclass);

	rt_Symbol = rt_class_create_unnamed(rt_Object);
	rt_String = rt_class_create_unnamed(rt_Object);

	rt_class_name(rt_Object, rt_Object, rt_symbol_from_cstr("Object"));
	rt_class_name(rt_Module, rt_Object, rt_symbol_from_cstr("Module"));
	rt_class_name(rt_Class, rt_Object, rt_symbol_from_cstr("Class"));
	rt_class_name(rt_Symbol, rt_Object, rt_symbol_from_cstr("Symbol"));
	rt_class_name(rt_String, rt_Object, rt_symbol_from_cstr("String"));

	rt_define_method(rt_Class, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_class_to_s);
	rt_define_method(rt_Object, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_object_to_s);
	rt_define_singleton_method(rt_Object, rt_symbol_from_cstr("allocate"), (rt_compiled_block_t)rt_object_allocate);

	rt_main = rt_object_allocate(rt_Object, 0);

	rt_define_singleton_method(rt_main, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_main_to_s);
}
