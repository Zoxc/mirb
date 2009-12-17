#include "classes.h"
#include "runtime.h"
#include "constant.h"
#include "classes/symbol.h"
#include "classes/string.h"
#include "classes/module.h"

rt_value rt_main;

khash_t(rt_hash) *object_var_hashes;

static inline bool rt_object_has_vars(rt_value obj)
{
	if (obj & RT_FLAG_FIXNUM)
		return false;
	else if (obj <= RT_MAX)
		return false;
	else
		switch(RT_COMMON(obj)->flags & RT_TYPE_MASK)
		{
			case C_CLASS:
			case C_MODULE:
			case C_OBJECT:
			case C_ICLASS:
				return true;

			default:
				return false;
		}
}

khash_t(rt_hash) *rt_object_get_vars(rt_value obj)
{
	if (rt_object_has_vars(obj))
	{
		return rt_get_vars(obj);
	}
	else
	{
		khiter_t k = kh_get(rt_hash, object_var_hashes, obj);

		if (k != kh_end(object_var_hashes))
			return (khash_t(rt_hash) *)kh_value(object_var_hashes, k);

		int ret;

		k = kh_put(rt_hash, object_var_hashes, obj, &ret);

		RT_ASSERT(ret);

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
	RT_OBJECT(c)->vars = 0;
	RT_CLASS(c)->super = super;
	RT_CLASS(c)->methods = 0;

	return c;
}

rt_value rt_module_create_bare()
{
	rt_value c = rt_alloc(sizeof(struct rt_class));

	RT_COMMON(c)->flags = C_MODULE;
	RT_COMMON(c)->class_of = rt_Module;
	RT_OBJECT(c)->vars = 0;
	RT_CLASS(c)->super = 0;
	RT_CLASS(c)->methods = 0;

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

	rt_value new_path;

	if(under == rt_Object)
	{
		new_path = rt_string_from_symbol(name);
	}
	else
	{
 		new_path = rt_dup_string(under_path);

		rt_concat_string(new_path, rt_string_from_cstr("::"));
		rt_concat_string(new_path, rt_string_from_symbol(name));
	}

	rt_object_set_var(obj, rt_symbol_from_cstr("__classname__"), rt_string_from_symbol(name));
	rt_object_set_var(obj, rt_symbol_from_cstr("__classpath__"), new_path);

	rt_const_set(under, name, obj);
}

rt_value rt_define_class(rt_value under, rt_value name, rt_value super)
{
	if(rt_const_defined(under, name))
		return rt_const_get(under, name);

	rt_value obj = rt_class_create_unnamed(super);

	rt_class_name(obj, under, name);

	#ifdef DEBUG
		printf("Defining class %s(%d) < %s(%d)\n", rt_string_to_cstr(rt_inspect(obj)), obj, rt_string_to_cstr(rt_inspect(super)), super);
	#endif

	return obj;
}

rt_value rt_define_module(rt_value under, rt_value name)
{
	if(rt_const_defined(under, name))
		return rt_const_get(under, name);

	rt_value obj = rt_module_create_bare();

	rt_class_name(obj, under, name);

	#ifdef DEBUG
		printf("Defining module %s(%d)\n", rt_string_to_cstr(rt_inspect(obj)), obj);
	#endif

	return obj;
}

rt_value rt_create_include_class(rt_value module, rt_value super)
{
	if(rt_type(module) == C_ICLASS)
		module = RT_COMMON(module)->class_of;

	rt_value c = rt_alloc(sizeof(struct rt_class));

	RT_COMMON(c)->flags = C_ICLASS;
	RT_COMMON(c)->class_of = module;
	RT_OBJECT(c)->vars = rt_get_vars(module);
	RT_CLASS(c)->super = super;
	RT_CLASS(c)->methods = rt_get_methods(module);

	return c;
}

void rt_include_module(rt_value obj, rt_value module)
{
	rt_value c = obj;

	while(module)
	{
		bool found_superclass = false;

		for (rt_value i = RT_CLASS(obj)->super; i; i = RT_CLASS(i)->super)
		{
			switch(rt_type(i))
			{
				case C_ICLASS:
					if(RT_OBJECT(i)->vars == RT_OBJECT(module)->vars)
					{
						if(!found_superclass)
							c = i;

						goto skip;
					}
					break;

				case C_CLASS:
					found_superclass = true;
					break;

				default:
					break;
			}
		}

		#ifdef DEBUG
			printf("Including module %s in %s\n", rt_string_to_cstr(rt_inspect(module)), rt_string_to_cstr(rt_inspect(obj)));
		#endif

		c = RT_CLASS(c)->super = rt_create_include_class(module, RT_CLASS(c)->super);

		skip:
			module = RT_CLASS(module)->super;
	}
}

void rt_define_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	rt_class_set_method(obj, name, block);

	#ifdef DEBUG
		printf("Defining method %s.%s\n", rt_string_to_cstr(rt_inspect(obj)), rt_symbol_to_cstr(name));
	#endif
}

void rt_define_singleton_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	rt_define_method(rt_singleton_class(obj), name, block);
}

/*
	main
*/

rt_compiled_block(rt_main_to_s)
{
	return rt_string_from_cstr("main");
}

rt_compiled_block(rt_main_include)
{
	return rt_module_include(0, RT_NIL, RT_NIL, rt_Object, block, argc, argv);
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
	rt_class_create_singleton(rt_Class, metaclass);

	rt_Symbol = rt_class_create_unnamed(rt_Object);
	rt_String = rt_class_create_unnamed(rt_Object);

	rt_class_name(rt_Object, rt_Object, rt_symbol_from_cstr("Object"));
	rt_class_name(rt_Module, rt_Object, rt_symbol_from_cstr("Module"));
	rt_class_name(rt_Class, rt_Object, rt_symbol_from_cstr("Class"));
	rt_class_name(rt_Symbol, rt_Object, rt_symbol_from_cstr("Symbol"));
	rt_class_name(rt_String, rt_Object, rt_symbol_from_cstr("String"));

	rt_main = rt_alloc_object(rt_Object);

	rt_define_singleton_method(rt_main, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_main_to_s);
	rt_define_singleton_method(rt_main, rt_symbol_from_cstr("include"), (rt_compiled_block_t)rt_main_include);
}
