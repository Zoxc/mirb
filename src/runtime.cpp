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
#include "classes/numeric.hpp"
#include "classes/integer.hpp"
#include "classes/true-class.hpp"
#include "classes/false-class.hpp"
#include "classes/nil-class.hpp"
#include "classes/string.hpp"
#include "classes/regexp.hpp"
#include "classes/proc.hpp"
#include "classes/array.hpp"
#include "classes/float.hpp"
#include "classes/exception.hpp"
#include "classes/exceptions.hpp"
#include "classes/io.hpp"
#include "classes/file.hpp"
#include "classes/dir.hpp"
#include "classes/range.hpp"
#include "modules/kernel.hpp"
#include "modules/comparable.hpp"
#include "modules/enumerable.hpp"
#include "modules/process.hpp"
#include "platform/platform.hpp"
#include "vm.hpp"
#include "context.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "tree/printer.hpp"
#endif

namespace Mirb
{
	prelude_thread size_t stack_stop;
	prelude_thread size_t stack_continue;

	void set_current_exception(Exception *exception)
	{
		if(exception)
		{
			Value::assert_valid(exception);
			Value::assert_valid(exception->instance_of);
		}

		context->exception = exception;

		if(exception)
			context->exception_frame_origin = context->frame;
		else
			context->exception_frame_origin = 0;
	}

	Class *class_of(value_t obj)
	{
		Value::assert_valid(obj);

		if(prelude_likely(Value::object_ref(obj)))
			return cast<Object>(obj)->instance_of;
		else
			return Value::class_of_literal(obj);
	}
	
	bool is_real_class(Class *obj)
	{
		return !obj->singleton && obj->get_type() != Value::IClass;
	}

	Class *real_class(Class *obj)
	{
		Value::assert_valid(obj);

		while(obj && !is_real_class(obj))
			obj = obj->superclass;

		return obj;
	}

	Class *real_class_of(value_t obj)
	{
		return real_class(class_of(obj));
	}
	
	bool subclass_of(Class *super, Class *c)
	{
		while(c)
		{
			c = real_class(c);

			if(!c)
				break;

			if(c == super)
				return true;

			c = c->superclass;
		}

		return false;
	}

	bool kind_of(Class *klass, value_t obj)
	{
		return subclass_of(klass, class_of(obj));
	}
	
	value_t define_common(value_t under, Symbol *name, Value::Type type)
	{
		if(prelude_unlikely(!Value::of_type<Module>(under)))
		{
			String *under_str = inspect(under);

			if(!under_str)
				return 0;

			raise(context->type_error, "Invalid constant scope '" + under_str->string + "'");
			return 0;
		}

		value_t existing = get_var_raw(under, name);

		if(prelude_unlikely(existing != value_raise))
		{
			if(Value::type(existing) != type)
			{
				String *existing_str = inspect(real_class_of(existing));

				if(!existing_str)
					return 0;

				raise(context->type_error,  "Constant already exists with type " + existing_str->string);

				return 0;
			}
			else
				return existing;
		}

		return value_undef;
	}

	Class *define_class(value_t under, Symbol *name, Class *super)
	{
		value_t result = define_common(under, name, Value::Class);

		if(result != value_undef)
			return cast<Class>(result);

		Class *obj = class_create_unnamed(super);
		
		class_name(obj, cast<Module>(under), name);

		return obj;
	}

	Module *module_create_bare()
	{
		return Collector::allocate<Module>(Value::Module, context->module_class, nullptr);
	}

	Module *define_module(value_t under, Symbol *name)
	{
		value_t result = define_common(under, name, Value::Module);

		if(result != value_undef)
			return cast<Module>(result);

		Module *obj = module_create_bare();
		
		class_name(obj, cast<Module>(under), name);

		return obj;
	}
	
	Class *define_class(const CharArray &name, Class *super)
	{
		return define_class(context->object_class, symbol_pool.get(name), super);
	}
	
	Module *define_module(const CharArray &name)
	{
		return define_module(context->object_class, symbol_pool.get(name));
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

			c = c->superclass = Class::create_include_class(module, c->superclass);

			skip:
				module = module->superclass;
		}
	}
	
	Class *singleton_class(value_t object)
	{
		Class *c = class_of(object);

		if(prelude_likely(c->singleton))
			return c;

		return class_create_singleton(object, c);
	}

	void class_name(value_t obj, Module *under, Symbol *name)
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
		
		class_create_singleton(obj, super->instance_of);

		return obj;
	}

	Class *class_create_bare(Class *super)
	{
		return Collector::allocate<Class>(Value::Class, context->class_class, super);
	}

	Class *class_create_singleton(value_t obj, Class *super)
	{
		Object *object = try_cast<Object>(obj);

		if(!object)
		{
			raise(context->type_error, "Unable to create singleton classes on immediate values.");
			return nullptr;
		}

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
		String *str = inspect(obj);
		
		if(!str)
		{
			swallow_exception();
			return CharArray("");
		}

		return str->string;
	}
	
	bool append_inspect(CharArray &result, value_t obj)
	{
		String *str = inspect(obj);

		if(str)
		{
			result += str->string;

			return true;
		}
		else
			return false;
	}
	
	std::string inspect_object(value_t obj)
	{
		String *str = inspect(obj);

		if(!str)
		{
			swallow_exception();
			return  "";
		}

		return str->get_string();
	}

	String *inspect(value_t obj)
	{
		OnStack<1> os(obj);

		Method *inspect = lookup_method(class_of(obj), Symbol::from_string("inspect"));

		value_t result = 0;

		if(inspect && (inspect != context->inspect_method || lookup_method(class_of(obj), Symbol::from_string("to_s"))))
		{
			result = call(obj, "inspect");

			if(!result)
				return 0;
		}

		if(!result)
			return String::get("#<?>");

		auto str_result = try_cast<String>(result);

		if(str_result)
			return str_result;
		else
			return cast<String>(Object::to_s(result));
	}

	CharArray pretty_inspect(value_t obj)
	{
		OnStack<1> os(obj);
		CharArray left = inspect_obj(obj);
		OnStackString<1> oss(left);
		CharArray right = inspect_obj(real_class_of(obj));

		return left + ":" + right;
	}

	ValueMap *get_vars(value_t obj)
	{
		Value::assert_valid(obj);
		
		Object *object = try_cast<Object>(obj);

		if(prelude_unlikely(!object))
			return context->dummy_map; // TODO: Turn a ValueMap per object

		if(prelude_unlikely(!object->vars))
			object->vars = Collector::allocate<ValueMap>();
		
		return object->vars;
	}
	
	Module *can_have_consts(value_t obj)
	{
		auto module = try_cast<Module>(obj);

		if(prelude_likely(module != 0))
			return module;
		else
		{
			String *obj_str = inspect(obj);

			if(!obj_str)
				return 0;

			raise(context->name_error, "Object " + obj_str->string + " can not contain constants");

			return 0;
		}
	}

	CharArray scope_path(Tuple<Module> *scope)
	{
		Value::assert_valid(scope);

		CharArray result;
		
		OnStack<1> os(scope);
		OnStackString<1> oss(result);

		for(size_t i = scope->entries; i-- > 0;)
		{
			result += inspect_obj((*scope)[i]) + "::";
		}

		return result;
	}
	
	value_t lookup_const(Module *module, Symbol *name)
	{
		module = module->superclass;

		while(module)
		{
			value_t value = get_var_raw(module, name);

			if(value != value_raise)
				return value;

			module = module->superclass;
		}
		
		return value_raise;
	}
	
	value_t test_const(Tuple<Module> *scope, Symbol *name)
	{
		Value::assert_valid(name);
		
		for(size_t i = 0; i < scope->entries; ++i)
		{
			value_t value = get_var_raw((*scope)[i], name);

			if(value != value_raise)
				return value;
		}

		return lookup_const(scope->first(), name);
	}
	
	value_t get_scoped_const(value_t obj, Symbol *name)
	{
		Value::assert_valid(obj);
		Value::assert_valid(name);

		auto module = can_have_consts(obj);

		if(!module)
			return 0;

		value_t value = get_var_raw(obj, name);

		if(value)
			return value;

		value = lookup_const(module, name);
		
		if(prelude_likely(value != nullptr))
			return value;

		OnStack<1> os(name);

		String *obj_str = inspect(obj);

		if(!obj_str)
			return 0;

		raise(context->name_error, "Uninitialized constant " + obj_str->string + "::" + name->string);

		return 0;
	}

	value_t get_const(Tuple<Module> *scope, Symbol *name)
	{
		Value::assert_valid(scope);
		Value::assert_valid(name);

		value_t result = test_const(scope, name);

		if(prelude_likely(result != value_raise))
			return result;
		
		raise(context->name_error, "Uninitialized constant " + scope_path(scope) + name->string);

		return value_raise;
	}

	value_t set_const(value_t obj, Symbol *name, value_t value)
	{
		Value::assert_valid(value);

		if(!can_have_consts(obj))
			return value_raise;
		
		set_var(obj, name, value);

		return value_true;
	}
	
	value_t get_var_raw(value_t obj, Symbol *name)
	{
		return ValueMapAccess::get(get_vars(obj), name, [] { return value_raise; });
	}

	value_t get_var(value_t obj, Symbol *name)
	{
		return ValueMapAccess::get(get_vars(obj), name, [] { return value_nil; });
	}

	void set_var(value_t obj, Symbol *name, value_t value)
	{
		Value::assert_valid(value);

		ValueMapAccess::set(get_vars(obj), name, value);
	}
	
	value_t get_global(Symbol *name)
	{
		return GlobalAccess::get(context, name, [] { return value_nil; });
	}

	void set_global(Symbol *name, value_t value)
	{
		GlobalAccess::set(context, name, value);
	}
	
	value_t compare(value_t left, value_t right)
	{
		value_t result = call_argv(left, context->syms.compare, value_nil, 1, &right);
		
		if(!result)
			return 0;

		if(!Value::is_fixnum(result))
			return raise(context->type_error, "<=> must return a Fixnum");

		return result;
	}

	value_t type_error(value_t value, const CharArray &expected)
	{
		OnStackString<1> os(expected);
		CharArray value_str = pretty_inspect(value);

		return raise(context->type_error, value_str + " was given when " + expected + " was expected");
	}

	bool type_error(value_t value, value_t expected)
	{
		value_t klass = real_class_of(value);

		if(prelude_unlikely(klass != expected))
		{
			CharArray value_str = pretty_inspect(value);

			OnStackString<1> os(value_str);

			CharArray expected_str = inspect_obj(expected);

			raise(context->type_error, value_str + " was given when a object of type " + expected_str + " was expected");
			return true;
		}
		else
			return false;
	}
	
	Exception *create_exception(Class *exception_class, const CharArray &message)
	{
		return Collector::allocate<Exception>(exception_class, message.to_string(), backtrace());
	}

	value_t raise(Class *exception_class, const CharArray &message)
	{
		set_current_exception(create_exception(exception_class, message));
		
		return value_raise;
	}

	value_t raise(Exception *exception)
	{
		set_current_exception(exception);

		return value_raise;
	}
	
	value_t eval(value_t self, Symbol *method_name, Tuple<Module> *scope, const char_t *input, size_t length, const CharArray &filename, bool free_input)
	{
		Block *block;

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

			Tree::Scope *tree_scope = parser.parse_main();

			if(!parser.messages.empty())
			{
				for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
					i().print();

				return raise(context->syntax_error, "Unable to parse file '" + filename + "'");
			}

			OnStack<3> os(tree_scope, method_name, scope);

			#ifdef MIRB_DEBUG_COMPILER
				DebugPrinter printer;

				std::cout << "Parsing done.\n-----\n";
				std::cout << printer.print_node(tree_scope->group);
				std::cout << "\n-----\n";
			#endif

			block = Compiler::compile(tree_scope, memory_pool);
		}

		return call_code(block, self, method_name, scope, value_nil, 0, 0);
	}
	
	Method *lookup_method(Module *module, Symbol *name)
	{
		do
		{
			Method *result = module->get_method(name);

			if(result)
				return result;

			module = module->superclass;
		}
		while(module != nullptr);

		return 0;
	}
	
	Method *lookup(value_t obj, Symbol *name)
	{
		Method *result = lookup_method(class_of(obj), name);

		if(prelude_unlikely(!result))
		{
			OnStack<1> os(name);

			CharArray obj_str = pretty_inspect(obj);

			raise(context->name_error, "Undefined method '" + name->string + "' for " + obj_str);
			return 0;
		}

		return result;
	}

	Method *lookup_super(Module *module, Symbol *name)
	{
		Method *result = lookup_method(module->superclass, name);

		if(prelude_unlikely(!result))
		{
			OnStack<1> os(name);

			CharArray module_value = pretty_inspect(module);

			raise(context->name_error, "No superclass method '" + name->string + "' for " + module_value);
			return 0;
		}

		return result;
	}

	bool validate_return(value_t &result)
	{
		OnStack<1> os(result);

		if(result != value_raise)
			Value::assert_valid(result);

		if(context->exception && result != value_raise)
		{
			Frame *current = context->frame->prev;

			while(true)
			{
				if(current == context->exception_frame_origin)
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
	
	bool stack_no_reserve(Frame &frame)
	{
		return ((size_t)&frame <= stack_continue);
	}

	value_t call_frame(Frame &frame)
	{
		if(frame.code->scope)
			Value::assert_valid(frame.code->scope);

		if(prelude_unlikely((size_t)&frame <= stack_stop))
		{
			raise(context->system_stack_error, "Stack overflow");
			return value_raise;
		}

		frame.prev = context->frame;
		context->frame = &frame;

		frame.vars = nullptr;

		if(prelude_unlikely(Collector::check()))
		{
			context->frame = frame.prev;

			#ifdef DEBUG
				context->exception_frame_origin = context->frame;
			#endif

			return value_raise;
		}
		
		if(prelude_unlikely(frame.argc < frame.code->min_args))
		{
			raise(context->argument_error, "Too few arguments passed to function (" + CharArray::uint(frame.code->min_args) + " required)");
			context->frame = frame.prev;
			return value_raise;
		}

		if(prelude_unlikely(frame.code->max_args != (size_t)-1 && frame.argc > frame.code->max_args))
		{
			raise(context->argument_error, "Too many arguments passed to function (max " + CharArray::uint(frame.code->max_args) + ")");
			context->frame = frame.prev;
			return value_raise;
		}

		value_t result = frame.code->executor(frame);
		
		#ifdef DEBUG
			if(!validate_return(result))
				context->exception_frame_origin = frame.prev;
		#endif

		context->frame = frame.prev;

		return result;
	}

	value_t call_code(Block *code, value_t obj, Symbol *name, Tuple<Module> *scope, value_t block, size_t argc, value_t argv[])
	{
		Frame frame;

		frame.code = code;
		frame.obj = obj;
		frame.name = name;
		frame.scope = scope;
		frame.block = block;
		frame.argc = argc;
		frame.argv = argv;
		frame.scopes = nullptr;

		return call_frame(frame);
	};
	
	value_t call_argv(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[])
	{
		Method *method = lookup(obj, name);

		if(prelude_unlikely(!method))
			return value_raise;

		return call_argv(method->block, obj, name, method->scope, block, argc, argv);
	}
	
	template<typename F> bool on_stack_argv(size_t argc, value_t argv[], F func)
	{
		void *stack_memory = alloca(sizeof(OnStackBlock<false>) + 2 * sizeof(value_t) * argc);

		if(prelude_unlikely(!stack_memory))
		{
			raise(context->runtime_error, "Unable to allocate stack memory");
			return false;
		}

		OnStackBlock<false> *os = new (stack_memory) OnStackBlock<false>();

		os->size = argc;

		value_t *os_array = (value_t *)(os + 1);

		for(size_t i = 0; i < argc; ++i)
		{
			os_array[i] = (value_t)&os_array[argc + i];
			os_array[argc + i] = argv[i];
		}

		func(&os_array[argc]);

		os->~OnStackBlock<false>();

		return true;
	}
	
	value_t call_argv(Block *code, value_t obj, Symbol *name, Tuple<Module> *scope, value_t block, size_t argc, value_t argv[])
	{
		value_t result;

		bool on_stack = on_stack_argv(argc, argv, [&](value_t *on_stack_argv) {
			result = call_code(code, obj, name, scope, block, argc, on_stack_argv);
		});

		if(on_stack)
			return result;
		else
			return 0;
	}
	
	Proc *get_proc(value_t obj)
	{
		if(prelude_unlikely(obj == value_nil))
		{
			raise(context->local_jump_error, "No block given");
			return 0;
		}

		return raise_cast<Proc>(obj);
	}
	
	value_t yield_argv(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		Proc *proc = get_proc(obj);

		if(prelude_unlikely(!proc))
			return value_raise;
		
		value_t result;

		bool on_stack = on_stack_argv(argc, argv, [&](value_t *on_stack_argv) {
			result = Proc::call(proc, block, argc, on_stack_argv);
		});

		if(on_stack)
			return result;
		else
			return 0;
	}
	
	value_t yield_argv(value_t obj, size_t argc, value_t argv[])
	{
		return yield_argv(obj, value_nil, argc, argv);
	}
	
	value_t yield(value_t obj)
	{
		return yield_argv(obj, value_nil, 0, 0);
	}
	
	Tuple<StackFrame> *backtrace(Frame *from)
	{
		size_t index = 0;
		
		Frame *start = from ? from : context->frame;

		Frame *current = start;
		
		while(current)
		{
			index++;

			current = current->prev;
		}

		auto &result = *Tuple<StackFrame>::allocate(index);
		
		current = start;
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
		auto str_obj = try_cast<String>(obj);

		if(prelude_likely(str_obj != 0))
			return str_obj;
		
		obj = call(obj, "to_s");

		if(!obj)
			swallow_exception();

		str_obj = try_cast<String>(obj);

		if(prelude_unlikely(!str_obj))
			return inspect(obj);

		return str_obj;
	}

	value_t main_to_s()
	{
		return String::get("main");
	}
	
	value_t main_include(size_t argc, value_t argv[])
	{
		return Module::include(context->object_class, argc, argv);
	}

	void setup_classes()
	{
		context->object_class = Class::create_initial(nullptr);
		context->module_class = Class::create_initial(context->object_class);
		context->class_class = Class::create_initial(context->module_class);
		
		Class *metaclass;
		
		context->syms.attached = Symbol::create_initial("__attached__");

		metaclass = class_create_singleton(context->object_class, context->class_class);
		metaclass = class_create_singleton(context->module_class, metaclass);
		class_create_singleton(context->class_class, metaclass);
		
		context->symbol_class = class_create_unnamed(context->object_class);

		context->syms.attached->instance_of = context->symbol_class;

		context->syms.classpath = Symbol::get("__classpath__");
		context->syms.classname = Symbol::get("__classname__");
		context->syms.compare = Symbol::get("<=>");
		context->syms.equal = Symbol::get("==");

		context->string_class = class_create_unnamed(context->object_class);
		
		class_name(context->object_class, context->object_class, Symbol::get("Object"));
		class_name(context->module_class, context->object_class, Symbol::get("Module"));
		class_name(context->class_class, context->object_class, Symbol::get("Class"));
		class_name(context->symbol_class, context->object_class, Symbol::get("Symbol"));
		class_name(context->string_class, context->object_class, Symbol::get("String"));
		
		context->setup(); // Required by simple define_class

		// Setup variables required by Value::initialize_class_table()
		
		context->nil_class = define_class("NilClass", context->object_class);
		context->false_class = define_class("FalseClass", context->object_class);
		context->true_class = define_class("TrueClass", context->object_class);
		context->numeric_class = define_class("Numeric", context->object_class);
		context->integer_class = define_class("Integer", context->numeric_class);
		context->fixnum_class = define_class("Fixnum", context->integer_class);

		Value::initialize_class_table();
	}

	void setup_main()
	{
		context->main = Collector::allocate<Object>(context->object_class);

		singleton_method(context->main, "to_s", &main_to_s);
		singleton_method<Arg::Count, Arg::Values>(context->main, "include", &main_include);
	}
	
	const size_t buffer_zone = 0x1000 * 16;
	
	void initialize_thread()
	{
		size_t stack_start = Platform::stack_start();
		size_t limit = Platform::stack_limit();
		
		mirb_runtime_assert(stack_start > limit);
		mirb_runtime_assert(limit > 3 * buffer_zone);
		
		stack_stop = stack_start - limit + buffer_zone;
		stack_continue = stack_stop + buffer_zone;
	}

	void initialize(bool console)
	{
		Collector::initialize();

		context = new Context;

		context->dummy_map = Collector::allocate<ValueMap>();
				
		initialize_thread();

		Value::initialize_type_table();
		
		Platform::initialize(console);

		Lexer::setup_jump_table();

		setup_classes();
		Method::initialize(); // Must be called before methods are defined

		Class::initialize();
		Object::initialize();
		Module::initialize();
		
		Kernel::initialize();
		
		TrueClass::initialize();
		FalseClass::initialize();
		NilClass::initialize();
		
		Comparable::initialize();
		Enumerable::initialize();

		Symbol::initialize();
		String::initialize();
		Numeric::initialize();
		Integer::initialize();
		Fixnum::initialize();
		Float::initialize();
		Proc::initialize();
		Array::initialize();
		Hash::initialize();
		Range::initialize();
		Exception::initialize();
		initialize_exceptions();
		
		Collector::enable_interrupts = true;
		
		IO::initialize();
		File::initialize();
		Dir::initialize();
		Regexp::initialize();
		
		Process::initialize();
		
		setup_main();

		set_const(context->object_class, Symbol::get("RUBY_ENGINE"), String::get("mirb"));
		set_const(context->object_class, Symbol::get("RUBY_VERSION"), String::get("1.9"));

#ifdef WIN32
		set_const(context->object_class, Symbol::get("RUBY_PLATFORM"), String::get("mirb-winapi"));
#else
		set_const(context->object_class, Symbol::get("RUBY_PLATFORM"), String::get("mirb-posix"));
#endif

		set_const(context->object_class, Symbol::get("ARGV"), Collector::allocate<Array>());
		set_const(context->object_class, Symbol::get("ENV"), Collector::allocate<Hash>());

		set_global(Symbol::get("$:"), Collector::allocate<Array>());

		set_global(Symbol::get("$stderr"), Collector::allocate<Object>(context->object_class));
		set_global(Symbol::get("$stdout"), Collector::allocate<Object>(context->object_class));
		set_global(Symbol::get("$stdin"), Collector::allocate<Object>(context->object_class));

		mirb_debug(Collector::collect());
	}

	void finalize()
	{
		for(size_t i = context->at_exits.size(); i-- > 0;)
			Proc::call(cast<Proc>(context->at_exits[i]), value_nil, 0, nullptr);

		Platform::finalize();

		Collector::free();
	}
};

