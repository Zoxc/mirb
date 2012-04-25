#include "runtime.hpp"
#include "symbol-pool.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "compiler.hpp"
#include "document.hpp"
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
#include "classes/exceptions.hpp"
#include "modules/kernel.hpp"
#include "generic/benchmark.hpp"
#include "vm.hpp"

#ifdef DEBUG
	#include "tree/printer.hpp"
#endif

namespace Mirb
{
	Exception *current_exception;
	Frame *current_exception_frame_origin;

	void set_current_exception(Exception *exception)
	{
		current_exception = exception;

		if(exception)
			current_exception_frame_origin = current_frame;
		else
			current_exception_frame_origin = 0;
	}

	value_t class_of(value_t obj)
	{
		if(prelude_likely(Value::object_ref(obj)))
			return cast<Object>(obj)->instance_of;
		else
			return Value::class_of_literal(obj);
	}

	value_t real_class(value_t obj)
	{
		while(obj && (cast<Class>(obj)->singleton || cast<Class>(obj)->get_type() == Value::IClass))
			obj = cast<Class>(obj)->superclass;

		return obj;
	}

	value_t real_class_of(value_t obj)
	{
		return real_class(class_of(obj));
	}

	value_t define_class(value_t under, Symbol *name, value_t super)
	{
		value_t existing = test_const(under, auto_cast(name));

		if(prelude_unlikely(existing != value_raise))
			return existing;

		value_t obj = class_create_unnamed(super);
		
		class_name(obj, under, name);

		return obj;
	}

	value_t define_class(value_t under, std::string name, value_t super)
	{
		return define_class(under, auto_cast(symbol_pool.get(name)), super);
	}
	
	value_t module_create_bare()
	{
		return auto_cast(Collector::allocate<Module>(Value::Module, Module::class_ref, nullptr));
	}

	value_t define_module(value_t under, Symbol *name)
	{
		value_t existing = test_const(under, name);

		if(prelude_unlikely(existing != value_raise))
			return existing;

		value_t obj = module_create_bare();
		
		class_name(obj, under, name);

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

		return auto_cast(Collector::allocate<Class>(module, super));
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
				{
					OnStack<3> os(module, c, obj);
					
					value_t including = Value::type(module) == Value::IClass ? cast<Module>(module)->instance_of : module;

					std::cout << "Including module " << inspect_object(including) << " in " << inspect_object(obj) << "\n";
				}
			#endif

			c = cast<Module>(c)->superclass = create_include_class(module, cast<Module>(c)->superclass);

			skip:
				module = cast<Module>(module)->superclass;
		}
	}
	
	value_t singleton_class(value_t object)
	{
		value_t c = class_of(object);

		if(prelude_likely(cast<Class>(c)->singleton))
			return c;

		return class_create_singleton(object, c);
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
		return auto_cast(Collector::allocate<Class>(Value::Class, Class::class_ref, super));
	}

	value_t class_create_singleton(value_t object, value_t super)
	{
		value_t singleton = class_create_bare(super);
		Class *singleton_class = cast<Class>(singleton);

		singleton_class->singleton = true;

		cast<Object>(object)->instance_of = singleton; // TODO: Fix the case when object is not a instance of Object

		set_var(singleton, Symbol::from_literal("__attached__"), object);

		if(cast<Object>(object)->get_type() == Value::Class)
		{
			singleton_class->instance_of = singleton;

			// TODO: Find out what this is about
			//if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
			//	RT_CLASS(singleton)->super = rt_class_real(RT_CLASS(object)->super)->class_of;
		}

		return singleton;
	}

	CharArray inspect_obj(value_t obj)
	{
		value_t str = inspect(obj);

		return cast<String>(str)->string;
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

		OnStack<1> os(obj);

		if(inspect && (inspect != Object::inspect_block || lookup_method(class_of(obj), Symbol::from_string("to_s"), &dummy)))
			result = call(obj, "inspect");

		if(prelude_likely(Value::type(result) == Value::String))
			return result;
		else
			return Object::to_s(obj);
	}

	CharArray pretty_inspect(value_t obj)
	{
		return inspect_obj(obj) + ":" + inspect_obj(real_class_of(obj));
	}

	ValueMap *get_vars(value_t obj)
	{
		if(prelude_unlikely(!Value::of_type<Object>(obj)))
			mirb_runtime_abort("No value map on objects passed by value"); // TODO: Fix this

		Object *object = auto_cast(obj);

		if(prelude_unlikely(!object->vars))
			object->vars = Collector::allocate<ValueMap>();
		
		return object->vars;
	}
	
	bool can_have_consts(value_t obj)
	{
		if(prelude_likely(Value::of_type<Module>(obj)))
			return true;
		else
		{
			raise(NameError::class_ref, "Object " + inspect_obj(obj) + " can not contain constants");

			return false;
		}
	}
	
	value_t test_const(value_t obj, Symbol *name)
	{
		// TODO: Fix constant lookup

		prelude_debug_assert(Value::of_type<Module>(obj));

		auto lookup = [&](value_t obj) -> value_t {
			while(obj)
			{
				ValueMap *vars = get_vars(obj);

				value_t value = vars->map.get(auto_cast(name));

				if(value != value_raise)
					return value;

				obj = cast<Module>(obj)->superclass;
			}

			return value_raise;
		};

		value_t result = lookup(obj);
		if(result != value_raise)
			return result;
			
		result = lookup(Object::class_ref);
		if(result != value_raise)
			return result;
		
		return value_raise;
	}

	value_t get_const(value_t obj, Symbol *name)
	{
		if(!can_have_consts(obj))
			return value_raise;

		value_t result = test_const(obj, name);

		if(prelude_likely(result != value_raise))
			return result;
		
		raise(NameError::class_ref, "Uninitialized constant " + inspect_obj(obj) + "::" + name->string);

		return value_raise;
	}

	value_t set_const(value_t obj, Symbol *name, value_t value)
	{
		if(!can_have_consts(obj))
			return value_raise;

		ValueMap *vars = get_vars(obj);

		value_t *old = vars->map.get_ref(auto_cast(name));

		if(prelude_unlikely(old != 0))
		{
			*old = value;
			return value_true;
		}
		else
		{
			set_var(obj, name, value);
			return value_false;
		}
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
		return get_vars(obj)->map.try_get(auto_cast(name), []{ return value_nil; });
	}

	void set_var(value_t obj, Symbol *name, value_t value)
	{
		get_vars(obj)->map.set(auto_cast(name), value);
	}
	
	Block *get_method(value_t obj, Symbol *name)
	{
		return auto_cast_null(cast<Module>(obj)->get_methods()->map.get(name));
	}

	void set_method(value_t obj, Symbol *name, Block *method)
	{
		return cast<Module>(obj)->get_methods()->map.set(name, auto_cast_null(method));
	}

	bool type_error(value_t value, value_t expected)
	{
		value_t klass = class_of(value);

		if(klass != expected)
		{
			raise(TypeError::class_ref, inspect_obj(value) + " is of class " + inspect_obj(class_of(value)) + " while " + inspect_obj(expected)  + " was expected");
			return true;
		}
		else
			return false;
	}

	value_t raise(value_t exception_class, const CharArray &message)
	{
		OnStack<2> os(exception_class, message);

		set_current_exception(Collector::allocate<Exception>(exception_class, message.to_string(), backtrace().to_string()));
		
		return value_raise;
	}

	value_t raise(value_t exception)
	{
		set_current_exception(auto_cast(exception));
		return value_raise;
	}
	
	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, CharArray &filename, bool free_input)
	{
		MemoryPool memory_pool;
		Document &document = *Collector::allocate<Document>();

		Parser parser(symbol_pool, memory_pool, document);

		if(!free_input)
			document.copy(input, length);
		else
		{
			document.data = input;
			document.length = length;
		}

		document.name = filename;

		parser.load();
		
 		Tree::Fragment fragment(0, Tree::Chunk::main_size);
		Tree::Scope *scope = parser.parse_main(&fragment);
	
		if(!parser.messages.empty())
		{
			for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
				std::cout << i().format() << "\n";

			return value_nil;
		}
		
		#ifdef MIRB_DEBUG_COMPILER
			DebugPrinter printer;
		
			std::cout << "Parsing done.\n-----\n";
			std::cout << printer.print_node(scope->group);
			std::cout << "\n-----\n";
		#endif
	
		Block *block = Compiler::compile(scope, memory_pool);

		return call_code(block, self, auto_cast(method_name), method_module, value_nil, 0, 0);
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
		while(module != nullptr);

		return 0;
	}
	
	Block *lookup_nothrow(value_t obj, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(class_of(obj), name, result_module);

		if(prelude_unlikely(!result))
			return 0;

		return result;
	}

	Block *lookup_super_nothrow(value_t module, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(cast<Class>(module)->superclass, name, result_module);

		if(prelude_unlikely(!result))
			return 0;

		return result;
	}

	Block *lookup(value_t obj, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(class_of(obj), name, result_module);

		if(prelude_unlikely(!result))
		{
			raise(NameError::class_ref, "Undefined method '" + name->string + "' for " + pretty_inspect(obj));
			return 0;
		}

		return result;
	}

	Block *lookup_super(value_t module, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(cast<Class>(module)->superclass, name, result_module);

		if(prelude_unlikely(!result))
		{
			raise(NameError::class_ref, "No superclass method '" + name->string + "' for " + pretty_inspect(module));
			return 0;
		}

		return result;
	}

	value_t call_frame(Frame &frame)
	{
		frame.prev = current_frame;
		current_frame = &frame;

		value_t result = frame.code->executor(frame);
		
		#ifdef DEBUG
			if(current_exception && result != value_raise)
			{
				Frame *current = frame.prev;

				while(true)
				{
					if(current == current_exception_frame_origin)
						goto exit;

					if(!current)
						break;

					current = current->prev;
				}

				std::cout << "Function raised exception but didn't indicate it:\n" << backtrace().get_string() << std::endl;
			}
			else
				current_exception_frame_origin = frame.prev;
			exit:
		#endif

		current_frame = frame.prev;

		return result;
	}

	value_t call_code(Block *code, value_t obj, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[])
	{
		Frame frame;

		frame.code = code;
		frame.obj = obj;
		frame.name = name;
		frame.module = module;
		frame.block = block;
		frame.argc = argc;
		frame.argv = argv;

		return call_frame(frame);
	};
	
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[])
	{
		value_t module;

		Block *method = lookup(obj, name, &module);

		if(prelude_unlikely(!method))
			return value_raise;

		return call_code(method, obj, name, module, block, argc, argv);
	}
	
	value_t yield(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		if(!Value::of_type<Proc>(obj))
		{
			raise(LocalJumpError::class_ref, "No block given");
			return value_raise;
		}

		return call(obj, "call", block, argc, argv);
	}
	
	value_t yield(value_t obj, size_t argc, value_t argv[])
	{
		return yield(obj, value_nil, argc, argv);
	}
	
	value_t yield(value_t obj)
	{
		return yield(obj, value_nil, 0, 0);
	}
	
	CharArray backtrace()
	{
		Frame *current = current_frame;

		CharArray result;

		OnStack<1> os(result);

		while(current)
		{
			if(result.size() != 0)
				result += "\n";

			result += current->inspect();

			current = current->prev;
		}

		return result;
	}
	
	String *enforce_string(value_t obj)
	{
		if(prelude_likely(Value::type(obj) == Value::String))
			return auto_cast(obj);
		
		obj = call(obj, "to_s");

		if(prelude_unlikely(Value::type(obj) != Value::String))
			return auto_cast(inspect(obj));

		return auto_cast(obj);
	}

	value_t main;
	
	value_t main_to_s()
	{
		return String::from_literal("main");
	}
	
	value_t main_include(size_t argc, value_t argv[])
	{
		return Module::include(Object::class_ref, argc, argv);
	}

	void setup_classes()
	{
		Object::class_ref = class_create_bare(nullptr);
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
		
		// Setup variables required by Value::initialize()
		
		NilClass::class_ref = define_class(Object::class_ref, "NilClass", Object::class_ref);
		FalseClass::class_ref = define_class(Object::class_ref, "FalseClass", Object::class_ref);
		TrueClass::class_ref = define_class(Object::class_ref, "TrueClass", Object::class_ref);
		Fixnum::class_ref = define_class(Object::class_ref, "Fixnum", Object::class_ref);

		Value::initialize();
		
		main = Object::allocate(Object::class_ref);

		singleton_method(main, "to_s", &main_to_s);
		singleton_method<Arg::Count, Arg::Values>(main, "include", &main_include);
	}
	
	void initialize()
	{
		std::cout << "Initialized in " << benchmark([] {
			Collector::initialize();
			Lexer::setup_jump_table();

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
			initialize_exceptions();
		}).format() << "\n";
	}

	void finalize()
	{
	}
};

