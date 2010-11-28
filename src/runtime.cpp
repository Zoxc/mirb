#include "runtime.hpp"
#include "symbol-pool.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "compiler.hpp"
#include "classes/object.hpp"
#include "classes/module.hpp"
#include "classes/symbol.hpp"
#include "classes/class.hpp"
#include "classes/fixnum.hpp"
#include "classes/true-class.hpp"
#include "classes/false-class.hpp"
#include "classes/nil-class.hpp"
#include "classes/string.hpp"
#include "classes/proc.hpp"
#include "classes/array.hpp"
#include "classes/exception.hpp"
#include "modules/kernel.hpp"
#include "generic/executable-heap.hpp"

#ifdef DEBUG
	#include "tree/printer.hpp"
#endif

namespace Mirb
{
	value_t class_of(value_t obj)
	{
		if(obj & 1)
			return Fixnum::class_ref;
		else if(obj <= value_highest)
		{
			switch(obj)
			{
				case value_true:
					return TrueClass::class_ref;

				case value_false:
					return FalseClass::class_ref;

				case value_nil:
					return NilClass::class_ref;

				default:
					Mirb::debug_fail("Unknown literal type");
			}
		}
		else
			return cast<Object>(obj)->instance_of;
	}

	value_t real_class(value_t obj)
	{
		while(obj && (cast<Class>(obj)->singleton || cast<Class>(obj)->type == Value::IClass))
			obj = cast<Class>(obj)->superclass;

		return obj;
	}

	value_t real_class_of(value_t obj)
	{
		return real_class(class_of(obj));
	}

	value_t define_class(value_t under, Symbol *name, value_t super)
	{
		value_t existing = get_const(under, auto_cast(name), false);

		if(existing != value_undef)
			return existing;

		value_t obj = class_create_unnamed(super);
		
		class_name(obj, under, name);

		#ifdef DEBUG
			std::cout << "Defining class " << inspect_object(obj) << "(" << CharArray::hex(obj).get_string() << ") < " << inspect_object(super) << "(" << CharArray::hex(super).get_string() << ")\n";
		#endif

		return obj;
	}

	value_t define_class(value_t under, std::string name, value_t super)
	{
		return define_class(under, auto_cast(symbol_pool.get(name)), super);
	}
	
	value_t module_create_bare()
	{
		return auto_cast(new (gc) Module(Value::Module, Module::class_ref, 0));
	}

	value_t define_module(value_t under, Symbol *name)
	{
		value_t existing = get_const(under, name, false);

		if(existing != value_undef)
			return existing;

		value_t obj = module_create_bare();
		
		class_name(obj, under, name);

		#ifdef DEBUG
			std::cout << "Defining module " << inspect_object(obj) << "(" << CharArray::hex(obj).get_string() << ")\n";
		#endif

		return obj;
	}

	value_t define_module(value_t under, std::string name)
	{
		return define_module(under, auto_cast(symbol_pool.get(name)));
	}
	
	value_t create_include_class(value_t module, value_t super)
	{
		if(Value::type(module) == Value::IClass)
			module = cast<Module>(module)->instance_of;

		return auto_cast(new (gc) Class(module, super));
	}

	void include_module(value_t obj, value_t module)
	{
		value_t c = obj;

		while(module)
		{
			bool found_superclass = false;

			for (value_t i = cast<Module>(obj)->superclass; i; i = cast<Module>(i)->superclass)
			{
				switch(Value::type(i))
				{
					case Value::IClass:
						if(cast<Module>(i)->vars == cast<Module>(module)->vars)
						{
							if(!found_superclass)
								c = i;

							goto skip;
						}
						break;

					case Value::Class:
						found_superclass = true;
						break;

					default:
						break;
				}
			}

			#ifdef DEBUG
				std::cout << "Including module " << inspect_object(module) << " in " << inspect_object(obj) << "\n";
			#endif

			c = cast<Module>(c)->superclass = create_include_class(module, cast<Module>(c)->superclass);

			skip:
				module = cast<Module>(module)->superclass;
		}
	}
	
	value_t singleton_class(value_t object)
	{
		value_t c = class_of(object);

		if(cast<Class>(c)->singleton)
			return c;

		return class_create_singleton(object, c);
	}

	void define_method(value_t obj, std::string name, compiled_block_t compiled_block)
	{
		Block *block = new (gc) Block;
		
		block->compiled = compiled_block;

		Symbol *name_as_symbol = Symbol::from_string(name);

		block->name = name_as_symbol;
	
		#ifdef DEBUG
			std::cout << "Defining method " << inspect_object(obj) << "." << name << "\n";
		#endif
		
		set_method(obj, name_as_symbol, block);
	}

	void define_singleton_method(value_t obj, std::string name, compiled_block_t compiled_block)
	{
		define_method(singleton_class(obj), name, compiled_block);
	}

	void class_name(value_t obj, value_t under, Symbol *name)
	{
		value_t under_path = get_var(under, Symbol::from_literal("__classpath__"));

		CharArray new_path;

		if(under == Object::class_ref)
		{
			new_path = name->string;
		}
		else
		{
			new_path = cast<String>(under_path)->string + "::" + name->string;
		}

		set_var(obj, Symbol::from_literal("__classname__"), String::from_symbol(name));
		set_var(obj, Symbol::from_literal("__classpath__"), new_path.to_string());

		set_const(under, name, obj);
	}

	value_t class_create_unnamed(value_t super)
	{
		value_t obj = class_create_bare(super);

		class_create_singleton(obj, cast<Object>(super)->instance_of);

		return obj;
	}

	value_t class_create_bare(value_t super)
	{
		return auto_cast(new (gc) Class(Value::Class, Class::class_ref, super));
	}

	value_t class_create_singleton(value_t object, value_t super)
	{
		value_t singleton = class_create_bare(super);
		Class *singleton_class = cast<Class>(singleton);

		singleton_class->singleton = true;

		cast<Object>(object)->instance_of = singleton; // TODO: Fix the case when object is not a instance of Object

		set_var(singleton, Symbol::from_literal("__attached__"), object);

		if(cast<Object>(object)->type == Value::Class)
		{
			singleton_class->instance_of = singleton;

			// TODO: Find out what this is about
			//if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
			//	RT_CLASS(singleton)->super = rt_class_real(RT_CLASS(object)->super)->class_of;
		}

		return singleton;
	}
	
	std::string inspect_object(value_t obj)
	{
		value_t str = inspect(obj);

		return cast<String>(str)->get_string();
	}

	value_t inspect(value_t obj)
	{
		value_t dummy;
		Block *inspect = lookup_method(class_of(obj), Symbol::from_string("inspect"), &dummy);

		value_t result = value_nil;

		if(inspect && (inspect->compiled != (Mirb::compiled_block_t)object_inspect || lookup_method(class_of(obj), Symbol::from_string("to_s"), &dummy)))
			result = call(obj, "inspect");

		if(Value::type(result) == Value::String)
			return result;
		else
			return object_to_s(value_nil, value_nil, obj, 0, 0, 0);
	}

	ValueMap *get_vars(value_t obj)
	{
		if(!Value::of_type<Object>(obj))
			Mirb::runtime_fail("No value map on objects passed by value"); // TODO: Fix this

		Object *object = auto_cast(obj);

		if(!object->vars)
			object->vars = new ValueMap(Object::vars_initial);
		
		return object->vars;
	}
	
	value_t get_const(value_t obj, Symbol *name, bool require)
	{
		// TODO: Fix constant lookup

		auto lookup = [&](value_t obj) -> value_t {
			while(obj)
			{
				ValueMap *vars = get_vars(obj);

				value_t value = vars->get(auto_cast(name));

				if(value != value_undef)
					return value;

				obj = cast<Class>(obj)->superclass;
			}

			return value_undef;
		};

		value_t result = lookup(obj);
		if(result != value_undef)
			return result;
			
		result = lookup(Object::class_ref);
		if(result != value_undef)
			return result;

		if(require)
			Mirb::debug_fail("Unable to find constant " + name->get_string() + " on " + inspect_object(obj));
		else
			return value_undef;
	}

	void set_const(value_t obj, Symbol *name, value_t value)
	{
		ValueMap *vars = get_vars(obj);

		value_t *old = vars->get_ref(auto_cast(name));

		if(old)
		{
			std::cout << "Warning: Reassigning constant " << name->get_string() << " to " << inspect_object(value) << "\n";
			*old = value;
		}
		else
			set_var(obj, name, value);
	}
	
	value_t get_ivar(value_t obj, Symbol *name)
	{
		return get_var(obj, name);
	}

	void set_ivar(value_t obj, Symbol *name, value_t value)
	{
		set_var(obj, name, value);
	}

	value_t get_var(value_t obj, Symbol *name)
	{
		return get_vars(obj)->try_get(auto_cast(name), []{ return value_nil; });
	}

	void set_var(value_t obj, Symbol *name, value_t value)
	{
		get_vars(obj)->set(auto_cast(name), value);
	}
	
	Block *get_method(value_t obj, Symbol *name)
	{
		return cast<Class>(obj)->get_methods()->get(name);
	}

	void set_method(value_t obj, Symbol *name, Block *method)
	{
		return cast<Class>(obj)->get_methods()->set(name, method);
	}
	
	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, CharArray &filename)
	{
		MemoryPool memory_pool;
		Parser parser(symbol_pool, memory_pool, filename);

		parser.load(input, length);
		
 		Tree::Fragment fragment(0, Tree::Chunk::main_size);
		Tree::Scope *scope = parser.parse_main(&fragment);
	
		if(!parser.messages.empty())
		{
			for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
				std::cout << i().format() << "\n";

			return value_nil;
		}
		
		#ifdef DEBUG
			DebugPrinter printer;
		
			std::cout << "Parsing done.\n-----\n";
			std::cout << printer.print_node(scope->group);
			std::cout << "\n-----\n";
		#endif
	
		Block *block = Compiler::compile(scope, memory_pool);

		value_t result = block->compiled(auto_cast(method_name), method_module, self, value_nil, 0, 0);

		block = 0; // Make sure block stays on stack*/

		return result;
	}
	
	Block *lookup_method(value_t module, Symbol *name, value_t *result_module)
	{
		do
		{
			Block *result = get_method(module, name);

			if(result)
			{
				*result_module = module;
				return result;
			}

			module = cast<Class>(module)->superclass;
		}
		while(module != 0);

		return 0;
	}
	
	compiled_block_t lookup(value_t obj, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(class_of(obj), name, result_module);

		if(!result)
			runtime_fail("Undefined method " + name->get_string() + " on " + inspect_object(obj));

		return result->compiled;
	}

	compiled_block_t lookup_super(value_t module, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(cast<Class>(module)->superclass, name, result_module);

		if(!result)
			runtime_fail("No superclass method " + name->get_string() + " for " + inspect_object(module));

		return result->compiled;
	}
	
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[])
	{
		value_t module;

		compiled_block_t method = lookup(obj, name, &module);

		return method(auto_cast(name), module, obj, block, argc, argv);
	}

	value_t call(value_t obj, const char *name, size_t argc, value_t argv[])
	{
		return call(obj, Symbol::from_string(name), value_nil, argc, argv);
	}

	value_t call(value_t obj, const char *name, value_t block, size_t argc, value_t argv[])
	{
		return call(obj, Symbol::from_string(name), block, argc, argv);
	}

	value_t call(value_t obj, Symbol *name, size_t argc, value_t argv[])
	{
		return call(obj, name, value_nil, argc, argv);
	}

	value_t call(value_t obj, Symbol *name)
	{
		return call(obj, name, value_nil, 0, 0);
	}

	value_t call(value_t obj, const char *name)
	{
		return call(obj, name, value_nil, 0, 0);
	}
	
	value_t main;

	mirb_compiled_block(main_to_s)
	{
		return String::from_literal("main");
	}

	mirb_compiled_block(main_include)
	{
		return module_include(value_nil, value_nil, Object::class_ref, block, argc, argv);
	}

	void setup_classes()
	{
		Object::class_ref = class_create_bare(0);
		Module::class_ref = class_create_bare(Object::class_ref);
		Class::class_ref = class_create_bare(Module::class_ref);

		value_t metaclass;

		metaclass = class_create_singleton(Object::class_ref, Class::class_ref);
		metaclass = class_create_singleton(Module::class_ref, metaclass);
		class_create_singleton(Class::class_ref, metaclass);

		Symbol::class_ref = class_create_unnamed(Object::class_ref);
		String::class_ref = class_create_unnamed(Object::class_ref);

		class_name(Object::class_ref, Object::class_ref, Symbol::from_literal("Object"));
		class_name(Module::class_ref, Object::class_ref, Symbol::from_literal("Module"));
		class_name(Class::class_ref, Object::class_ref, Symbol::from_literal("Class"));
		class_name(Symbol::class_ref, Object::class_ref, Symbol::from_literal("Symbol"));
		class_name(String::class_ref, Object::class_ref, Symbol::from_literal("String"));

		main = Object::allocate(Object::class_ref);

		define_singleton_method(main, "to_s", &main_to_s);
		define_singleton_method(main, "include", &main_include);
	}
	
	void initialize()
	{
		Lexer::setup_jump_table();
		ExecutableHeap::initialize();

		setup_classes();
		
		Class::initialize();
		Object::initialize();
		Module::initialize();
		
		Kernel::initialize();
		
		TrueClass::initialize();
		FalseClass::initialize();
		NilClass::initialize();

		Symbol::initialize();
		String::initialize();
		Fixnum::initialize();
		Proc::initialize();
		Array::initialize();
		Exception::initialize();
	}

	void finalize()
	{
		ExecutableHeap::finalize();
	}
};

