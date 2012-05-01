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
#include "classes/hash.hpp"
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
#include "platform/platform.hpp"
#include "vm.hpp"
#include "context.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "tree/printer.hpp"
#endif

namespace Mirb
{
	Exception *current_exception;
	Frame *current_exception_frame_origin;

	void set_current_exception(Exception *exception)
	{
		if(exception)
			Value::assert_valid(exception->instance_of);

		current_exception = exception;

		if(exception)
			current_exception_frame_origin = current_frame;
		else
			current_exception_frame_origin = 0;
	}

	value_t class_of(value_t obj)
	{
		Value::assert_valid(obj);

		if(prelude_likely(Value::object_ref(obj)))
			return cast<Object>(obj)->instance_of;
		else
			return Value::class_of_literal(obj);
	}

	value_t real_class(value_t obj)
	{
		Value::assert_valid(obj);

		while(obj && (cast<Class>(obj)->singleton || cast<Class>(obj)->get_type() == Value::IClass))
			obj = cast<Class>(obj)->superclass;

		return obj;
	}

	value_t real_class_of(value_t obj)
	{
		return real_class(class_of(obj));
	}

	Class *define_class(Module *under, Symbol *name, Class *super)
	{
		value_t existing = test_const(under, auto_cast(name));
		
		if(prelude_unlikely(existing != value_raise))
		{
			if(type(existing) != Value::Class)
			{
				raise(context->standard_error,  "Constant exists already");

				return nullptr;
			}
			else
				return auto_cast(existing);
		}

		Class *obj = class_create_unnamed(auto_cast(super));
		
		class_name(obj, under, name);

		return obj;
	}

	Class *define_class(Module *under, std::string name, Class *super)
	{
		return define_class(under, auto_cast(symbol_pool.get(name)), super);
	}
	
	Module *module_create_bare()
	{
		return Collector::allocate<Module>(Value::Module, context->module_class, nullptr);
	}

	Module *define_module(Module *under, Symbol *name)
	{
		value_t existing = test_const(under, name);

		if(prelude_unlikely(existing != value_raise))
		{
			if(type(existing) != Value::Module)
			{
				raise(context->standard_error,  "Constant exists already");

				return nullptr;
			}
			else
				return auto_cast(existing);
		}

		Module *obj = module_create_bare();
		
		class_name(obj, under, name);

		return obj;
	}

	Module *define_module(Module *under, std::string name)
	{
		return define_module(under, cast<Symbol>(symbol_pool.get(name)));
	}
	
	Class *create_include_class(Module *module, Class *super)
	{
		if(Value::type(module) == Value::IClass)
			module = module->instance_of;

		return Collector::allocate<Class>(module, super);
	}

	void include_module(Module *obj, Module *module)
	{
		Module *c = obj;

		while(module)
		{
			bool found_superclass = false;

			for (Class *i = obj->superclass; i; i = i->superclass)
			{
				switch(Value::type(i))
				{
					case Value::IClass:
						if(i->vars == module->vars)
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
					value_t including = Value::type(module) == Value::IClass ? module->instance_of : module;

					OnStack<4> os(obj, c, module, including);

					std::cout << "Including module " << inspect_object(including) << " in " << inspect_object(obj) << "\n";

					swallow_exception();
				}
			#endif

			c = c->superclass = create_include_class(module, c->superclass);

			skip:
				module = module->superclass;
		}
	}
	
	Class *singleton_class(Object *object)
	{
		Class *c = auto_cast(class_of(object));

		if(prelude_likely(c->singleton))
			return c;

		return class_create_singleton(object, c);
	}

	void class_name(value_t  obj, Module *under, Symbol *name)
	{
		value_t under_path = get_var(under, context->syms.classpath);
		
		CharArray new_path;
		
		if(under == context->object_class)
		{
			new_path = name->string;
		}
		else
		{
			new_path = cast<String>(under_path)->string + "::" + name->string;
		}

		set_var(obj, context->syms.classname, String::from_symbol(name));
		set_var(obj, context->syms.classpath, new_path.to_string());

		set_const(under, name, obj);
	}

	Class *class_create_unnamed(Class *super)
	{
		Class *obj = class_create_bare(super);
		
		class_create_singleton(obj, auto_cast(super->instance_of));

		return obj;
	}

	Class *class_create_bare(Class *super)
	{
		return Collector::allocate<Class>(Value::Class, context->class_class, super);
	}

	Class *class_create_singleton(Object *object, Class *super)
	{
		Class *singleton_class = class_create_bare(super);
		
		singleton_class->singleton = true;

		object->instance_of = singleton_class; // TODO: Fix the case when object is not a instance of Object

		set_var(singleton_class, context->syms.attached, object);

		if(object->get_type() == Value::Class)
		{
			singleton_class->instance_of = singleton_class;

			// TODO: Find out what this is about
			//if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
			//	RT_CLASS(singleton)->super = rt_class_real(RT_CLASS(object)->super)->class_of;
		}

		return singleton_class;
	}

	CharArray inspect_obj(value_t obj)
	{
		value_t str = inspect(obj);
		
		if(!str)
		{
			swallow_exception();
			return "";
		}

		return cast<String>(str)->string;
	}
	
	bool append_inspect(CharArray &result, value_t obj)
	{
		value_t str = inspect(obj);

		if(str)
		{
			result += cast<String>(str)->string;

			return true;
		}
		else
			return false;
	}
	
	std::string inspect_object(value_t obj)
	{
		value_t str = inspect(obj);

		if(!str)
		{
			swallow_exception();
			return  "";
		}

		return cast<String>(str)->get_string();
	}

	value_t inspect(value_t obj)
	{
		OnStack<1> os(obj);

		value_t dummy;
		Block *inspect = lookup_method(class_of(obj), Symbol::from_string("inspect"), &dummy);

		value_t result = value_nil;

		if(inspect && (inspect != Object::inspect_block || lookup_method(class_of(obj), Symbol::from_string("to_s"), &dummy)))
			result = call(obj, "inspect");

		if(!result)
			return 0;

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
		Value::assert_valid(obj);

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
			raise(context->name_error, "Object " + inspect_obj(obj) + " can not contain constants");

			return false;
		}
	}
	
	value_t test_const(value_t obj, Symbol *name)
	{
		Value::assert_valid(name);

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
			
		result = lookup(context->object_class);
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
		
		raise(context->name_error, "Uninitialized constant " + inspect_obj(obj) + "::" + name->string);

		return value_raise;
	}

	value_t set_const(value_t obj, Symbol *name, value_t value)
	{
		Value::assert_valid(value);

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
		Value::assert_valid(value);

		get_vars(obj)->map.set(auto_cast(name), value);
	}
	
	Block *get_method(value_t obj, Symbol *name)
	{
		return auto_cast_null(cast<Module>(obj)->get_methods()->map.get(name));
	}

	void set_method(value_t obj, Symbol *name, Block *method)
	{
		Value::assert_valid(method);

		return cast<Module>(obj)->get_methods()->map.set(name, auto_cast_null(method));
	}

	bool type_error(value_t value, value_t expected)
	{
		value_t klass = class_of(value);

		if(klass != expected)
		{
			raise(context->type_error, inspect_obj(value) + " is of class " + inspect_obj(class_of(value)) + " while " + inspect_obj(expected)  + " was expected");
			return true;
		}
		else
			return false;
	}

	value_t raise(Class *exception_class, const CharArray &message)
	{
		set_current_exception(Collector::allocate<Exception>(exception_class, message.to_string(), backtrace()));
		
		return value_raise;
	}

	value_t raise(value_t exception)
	{
		set_current_exception(auto_cast(exception));

		return value_raise;
	}
	
	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, CharArray &filename, bool free_input)
	{
		MemoryPool::Base memory_pool;
		Document *document = Collector::allocate_pinned<Document>();

		Parser parser(symbol_pool, memory_pool, document);

		if(!free_input)
			document->copy(input, length);
		else
		{
			document->data = input;
			document->length = length;
		}

		document->name = filename;

		parser.load();
		
		Tree::Scope *scope = parser.parse_main();
	
		if(!parser.messages.empty())
		{
			for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
				i().print();

			return value_nil;
		}

		OnStack<2> os(scope, document);
		
		#ifdef MIRB_DEBUG_COMPILER
			DebugPrinter printer;
		
			std::cout << "Parsing done.\n-----\n";
			std::cout << printer.print_node(scope->group);
			std::cout << "\n-----\n";
		#endif
	
		Block *block = Compiler::compile(scope, memory_pool);

		return call_code(block, self, method_name, method_module, value_nil, 0, 0);
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
			raise(context->name_error, "Undefined method '" + name->string + "' for " + pretty_inspect(obj));
			return 0;
		}

		return result;
	}

	Block *lookup_super(value_t module, Symbol *name, value_t *result_module)
	{
		Mirb::Block *result = lookup_method(cast<Class>(module)->superclass, name, result_module);

		if(prelude_unlikely(!result))
		{
			raise(context->name_error, "No superclass method '" + name->string + "' for " + pretty_inspect(module));
			return 0;
		}

		return result;
	}

	bool validate_return(value_t &result)
	{
		OnStack<1> os(result);

		if(result != value_raise)
			Value::assert_valid(result);

		if(current_exception && result != value_raise)
		{
			Frame *current = current_frame->prev;

			while(true)
			{
				if(current == current_exception_frame_origin)
					return true;

				if(!current)
					break;

				current = current->prev;
			}

			std::cerr << "Function raised exception but didn't indicate it:\n" << StackFrame::get_backtrace(backtrace()).get_string() << std::endl;

			result = value_raise;
		}

		return false;
	}

	value_t call_frame(Frame &frame)
	{
		if(frame.code->scope)
			Value::assert_valid(frame.code->scope);

		frame.prev = current_frame;
		current_frame = &frame;

		frame.vars = nullptr;

		if(prelude_unlikely(Collector::check()))
		{
			current_frame = frame.prev;

			#ifdef DEBUG
				current_exception_frame_origin = current_frame;
			#endif

			return value_raise;
		}

		value_t result = frame.code->executor(frame);
		
		#ifdef DEBUG
			if(!validate_return(result))
				current_exception_frame_origin = frame.prev;
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
		frame.scopes = nullptr;

		return call_frame(frame);
	};

	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[])
	{
		value_t module;

		Block *method = lookup(obj, name, &module);

		if(prelude_unlikely(!method))
			return value_raise;

		void *stack_memory = std::malloc(sizeof(OnStackBlock<false>) + 2 * sizeof(value_t) * argc);

		if(prelude_unlikely(!stack_memory))
			return raise(context->exception_class);

		OnStackBlock<false> *os = new (stack_memory) OnStackBlock<false>();

		os->size = argc;

		value_t *os_array = (value_t *)(os + 1);

		for(size_t i = 0; i < argc; ++i)
		{
			os_array[i] = (value_t)&os_array[argc + i];
			os_array[argc + i] = argv[i];
		}

		value_t result = call_code(method, obj, name, module, block, argc, &os_array[argc]);

		os->~OnStackBlock<false>();

		std::free(stack_memory);

		return result;
	}
	
	value_t yield(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		if(!Value::of_type<Proc>(obj))
		{
			raise(context->local_jump_error, "No block given");
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
	
	Tuple<StackFrame> *backtrace()
	{
		size_t index = 0;

		Frame *current = current_frame;
		
		while(current)
		{
			index++;

			current = current->prev;
		}

		auto &result = *Tuple<StackFrame>::allocate(index);
		
		current = current_frame;
		index = 0;

		while(current)
		{
			StackFrame *frame = Collector::allocate<StackFrame>(current);

			result[index++] = frame;
			current = current->prev;
		}

		return &result;
	}

	void swallow_exception()
	{
		set_current_exception(nullptr);
	}
	
	String *enforce_string(value_t obj)
	{
		if(prelude_likely(Value::type(obj) == Value::String))
			return auto_cast(obj);
		
		obj = call(obj, "to_s");

		if(!obj)
			swallow_exception();

		if(prelude_unlikely(Value::type(obj) != Value::String))
			return auto_cast(inspect(obj));

		return auto_cast(obj);
	}

	value_t main_to_s()
	{
		return String::from_literal("main");
	}
	
	value_t main_include(size_t argc, value_t argv[])
	{
		return Module::include(context->object_class, argc, argv);
	}

	void setup_classes()
	{
		context->object_class = class_create_bare(nullptr);
		context->module_class = class_create_bare(context->object_class);
		context->class_class = class_create_bare(context->module_class);
		
		context->syms.classpath = Symbol::from_literal("__classpath__");
		context->syms.classname = Symbol::from_literal("__classname__");
		context->syms.attached =  Symbol::from_literal("__attached__");

		Class *metaclass;

		metaclass = class_create_singleton(auto_cast(context->object_class), auto_cast(context->class_class));
		metaclass = class_create_singleton(auto_cast(context->module_class), metaclass);
		class_create_singleton(auto_cast(context->class_class), metaclass);

		context->symbol_class = class_create_unnamed(context->object_class);

		fix_symbol_pool();

		context->string_class = class_create_unnamed(context->object_class);
		
		class_name(context->object_class, context->object_class, Symbol::from_literal("Object"));
		class_name(context->module_class, context->object_class, Symbol::from_literal("Module"));
		class_name(context->class_class, context->object_class, Symbol::from_literal("Class"));
		class_name(context->symbol_class, context->object_class, Symbol::from_literal("Symbol"));
		class_name(context->string_class, context->object_class, Symbol::from_literal("String"));
		
		// Setup variables required by Value::initialize_class_table()
		
		context->nil_class = define_class(context->object_class, "NilClass", context->object_class);
		context->false_class = define_class(context->object_class, "FalseClass", context->object_class);
		context->true_class = define_class(context->object_class, "TrueClass", context->object_class);
		context->fixnum_class = define_class(context->object_class, "Fixnum", context->object_class);

		Value::initialize_class_table();
		
		context->main = Collector::allocate<Object>(context->object_class);

		singleton_method(context->main, "to_s", &main_to_s);
		singleton_method<Arg::Count, Arg::Values>(context->main, "include", &main_include);
	}
	
	void initialize()
	{
		std::cout << "Initialized in " << Platform::benchmark([] {
			context = new Context;
				
			Value::initialize_type_table();
		
			Platform::initialize();

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
			Hash::initialize();
			Exception::initialize();
			initialize_exceptions();

			Collector::enable_interrupts = true;

			mirb_debug(Collector::collect());
		}).format() << "\n";
	}

	void finalize()
	{
		Collector::free();
	}
};

