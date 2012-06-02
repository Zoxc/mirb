#include "runtime.hpp"
#include "symbol-pool.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "compiler.hpp"
#include "document.hpp"
#include "global.hpp"
#include "classes.hpp"
#include "number.hpp"
#include "stream.hpp"
#include "modules/kernel.hpp"
#include "modules/comparable.hpp"
#include "modules/enumerable.hpp"
#include "platform/platform.hpp"
#include "vm.hpp"
#include "context.hpp"
#include "recursion-detector.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "tree/printer.hpp"
#endif

namespace Mirb
{
	#define MIRB_YIELD(i) \
		value_t yield(value_t obj MIRB_COMMA_BEFORE_LIST(MIRB_CALL_ARG, i)) \
		{ \
			value_t argv[i]; \
			MIRB_LOCAL_STATEMENT_LIST(MIRB_CALL_STATEMENT, i) \
			return yield_argv(obj, i, argv); \
		};

	MIRB_STATEMENT_LIST_NO_ZERO(MIRB_YIELD, MIRB_STATEMENT_MAX);
	
	prelude_thread size_t stack_stop;
	prelude_thread size_t stack_continue;

	bool map_index(intptr_t index, size_t size, size_t &result)
	{
		if(index < 0)
		{
			index = -index;

			if((size_t)index > size)
			{
				return false;
			}

			result = size - (size_t)index;

			return true;
		}
		
		if((size_t)index >= size)
			return false;

		result = (size_t)index;
		
		return true;
	}
	
	void coerce_error(value_t left, value_t right)
	{
		OnStack<1> os(left);

		CharArray left_str = pretty_inspect(left);
		
		OnStackString<1> oss(left_str);
		
		CharArray right_str = pretty_inspect(right);
		
		raise(context->type_error, "Unable to coerce values " + left_str + " and " + right_str);
	}

	value_t coerce(value_t left, Symbol *name, value_t right)
	{
		auto result = raise_cast<Array>(call(right, "coerce", left));

		if(result->size() != 2)
			raise(context->runtime_error, "Expected an array with a pair of values");

		return call(result->get(0), name, result->get(1));
	}

	Class *internal_class_of(value_t obj)
	{
		obj->assert_valid();

		if(prelude_likely(obj->object_ref()))
			return cast<Object>(obj)->instance_of;
		else
			return obj->class_of_literal();
	}
	
	bool is_real_class(Class *obj)
	{
		return !obj->singleton && obj->value_type != Type::IClass;
	}

	Class *real_class(Class *obj)
	{
		obj->assert_valid();

		while(obj && !is_real_class(obj))
			obj = obj->superclass;

		return obj;
	}

	Class *class_of(value_t obj)
	{
		return real_class(internal_class_of(obj));
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
		return subclass_of(klass, internal_class_of(obj));
	}
	
	value_t define_common(value_t under, Symbol *name, Type::Enum type)
	{
		if(prelude_unlikely(!of_type<Module>(under)))
		{
			auto under_str = inspect(under);

			raise(context->type_error, "Invalid constant scope '" + under_str + "'");
		}

		value_t existing = get_var_raw(under, name);

		if(prelude_unlikely(existing != value_undef))
		{
			if(existing->type() != type)
			{
				auto existing_str = inspect(class_of(existing));

				raise(context->type_error,  "Constant already exists with type " + existing_str);
			}
			else
				return existing;
		}

		return 0;
	}

	Class *define_class(value_t under, Symbol *name, Class *super)
	{
		value_t result = define_common(under, name, Type::Class);

		if(result)
			return cast<Class>(result);

		Class *obj = class_create_unnamed(super);
		
		class_name(obj, cast<Module>(under), name);

		return obj;
	}

	Module *module_create_bare()
	{
		return Collector::allocate<Module>(Type::Module, context->module_class, nullptr);
	}

	Module *define_module(value_t under, Symbol *name)
	{
		value_t result = define_common(under, name, Type::Module);

		if(result)
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
				switch(i->type())
				{
					case Type::IClass:
						if(i->vars == module->vars)
						{
							if(!found_superclass)
								c = i;

							goto skip;
						}
						break;

					case Type::Class:
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
		Class *c = internal_class_of(object);

		if(prelude_likely(c->singleton))
			return c;

		return class_create_singleton(object, c);
	}

	void class_name(value_t obj, Module *under, Symbol *name)
	{
		String *under_path = try_cast<String>(get_var(under, context->syms.classpath));
		
		CharArray new_path;
		
		if(under == context->object_class)
		{
			new_path = name->string;
		}
		else
		{
			if(!under_path)
				return;
		
			new_path = under_path->string + "::" + name->string;
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
		return Collector::allocate<Class>(Type::Class, context->class_class, super);
	}

	Class *class_create_singleton(value_t obj, Class *super)
	{
		Object *object = try_cast<Object>(obj);

		if(!object)
			raise(context->type_error, "Unable to create singleton classes on immediate values.");

		Class *singleton_class = class_create_bare(super);
		
		singleton_class->singleton = true;

		object->instance_of = singleton_class;

		set_var(singleton_class, context->syms.attached, object);

		if(object->type() == Type::Class)
		{
			singleton_class->instance_of = singleton_class;

			// TODO: Find out what this is about
			//if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
			//	RT_CLASS(singleton)->super = rt_class_real(RT_CLASS(object)->super)->class_of;
		}

		return singleton_class;
	}
	
	CharArray rescue_inspect(value_t obj)
	{
		try
		{
			return inspect(obj);
		}
		catch(InternalException e)
		{
			if(kind_of(context->standard_error, e.value))
				return "<error: " + inspect(e.value) + ">";
			else
				throw;
		}
	}
	CharArray inspect(value_t obj)
	{
		return inspect_obj(obj)->string.trim(300, "<....>");
	}
	
	String *inspect_obj(value_t obj)
	{
		OnStack<1> os(obj);

		RecursionDetector<RecursionType::Inspect, true, 1> rd(obj);

		Method *inspect = respond_to(obj, "inspect");
		Method *to_s = respond_to(obj, "to_s");

		value_t result = 0;

		if(inspect && (inspect != context->inspect_method))
			result = call_argv(inspect, obj, Symbol::get("inspect"), value_nil, 0, 0);
		else if(to_s)
			result = call_argv(to_s, obj, Symbol::get("to_s"), value_nil, 0, 0);

		if(!result)
			return String::get("#<?>");

		auto str_result = try_cast<String>(result);

		if(str_result)
			return str_result;
		else
			return cast<String>(Object::to_s(obj));
	}

	CharArray pretty_inspect(value_t obj)
	{
		OnStack<1> os(obj);
		CharArray left = inspect(obj);
		OnStackString<1> oss(left);
		CharArray right = inspect(class_of(obj));

		return left + ":" + right;
	}

	ValueMap *get_vars(value_t obj)
	{
		obj->assert_valid();
		
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
			auto obj_str = inspect(obj);

			raise(context->name_error, "Object " + obj_str + " can not contain constants");
		}
	}

	CharArray scope_path(Tuple<Module> *scope)
	{
		scope->assert_valid();

		CharArray result;
		
		OnStack<1> os(scope);
		OnStackString<1> oss(result);

		for(size_t i = scope->entries; i-- > 0;)
		{
			result += inspect((*scope)[i]) + "::";
		}

		return result;
	}
	
	value_t lookup_const(Module *module, Symbol *name)
	{
		module = module->superclass;

		while(module)
		{
			value_t value = get_var_raw(module, name);

			if(prelude_likely(value != value_undef))
				return value;

			module = module->superclass;
		}
		
		return value_undef;
	}
	
	value_t test_const(Tuple<Module> *scope, Symbol *name)
	{
		name->assert_valid();
		
		for(size_t i = 0; i < scope->entries; ++i)
		{
			value_t value = get_var_raw((*scope)[i], name);

			if(prelude_likely(value != value_undef))
				return value;
		}

		return lookup_const(scope->first(), name);
	}
	
	value_t get_scoped_const(value_t obj, Symbol *name)
	{
		obj->assert_valid();
		name->assert_valid();

		auto module = can_have_consts(obj);

		value_t value = get_var_raw(obj, name);

		if(prelude_likely(value != value_undef))
			return value;

		value = lookup_const(module, name);
		
		if(prelude_likely(value != value_undef))
			return value;

		OnStack<1> os(name);

		auto obj_str = inspect(obj);

		raise(context->name_error, "Uninitialized constant " + obj_str + "::" + name->string);
	}

	value_t get_const(Tuple<Module> *scope, Symbol *name)
	{
		scope->assert_valid();
		name->assert_valid();

		value_t result = test_const(scope, name);

		if(prelude_likely(result != value_undef))
			return result;
		
		raise(context->name_error, "Uninitialized constant " + scope_path(scope) + name->string);
	}

	void set_const(value_t obj, Symbol *name, value_t value)
	{
		value->assert_valid();

		can_have_consts(obj);

		set_var(obj, name, value);
	}
	
	value_t get_var_raw(value_t obj, Symbol *name)
	{
		return ValueMapAccess::get(get_vars(obj), name, [] { return value_undef; });
	}

	value_t get_var(value_t obj, Symbol *name)
	{
		return ValueMapAccess::get(get_vars(obj), name, [] { return value_nil; });
	}

	void set_var(value_t obj, Symbol *name, value_t value)
	{
		value->assert_valid();

		ValueMapAccess::set(get_vars(obj), name, value);
	}
	
	Global *get_global_object(Symbol *name, bool force)
	{
		auto result = cast_null<Global>(GlobalAccess::get(context, name, [] { return nullptr; }));

		if(!result && force)
		{
			result = new (collector) Global;
			GlobalAccess::set(context, name, result);
		}

		return result;
	}
	
	void set_global_object(Symbol *name, Global *global)
	{
		GlobalAccess::set(context, name, global);
	}

	value_t get_global(Symbol *name)
	{
		auto global = get_global_object(name);

		if(!global)
			return value_nil;

		return global->get(name);
	}

	void set_global(Symbol *name, value_t value)
	{
		auto global = get_global_object(name, true);

		global->set(name, value);
	}
	
	int compare(value_t left, value_t right)
	{
		value_t result = call(left, context->syms.compare, right);
		
		if(!Value::is_fixnum(result))
			raise(context->type_error, "<=> must return a Fixnum between -1 and 1");

		intptr_t value = Fixnum::to_int(result);
		
		if((value < -1) || (value > 1))
			raise(context->type_error, "<=> must return a Fixnum between -1 and 1");

		return value;
	}
	
	void zero_division_error()
	{
		raise(context->zero_division_error, "Division by zero");
	}

	void type_error(value_t value, const CharArray &expected)
	{
		OnStackString<1> os(expected);
		CharArray value_str = pretty_inspect(value);

		raise(context->type_error, value_str + " was given when " + expected + " was expected");
	}

	void type_error(value_t value, Class *expected)
	{
		if(prelude_unlikely(!kind_of(expected, value)))
		{
			CharArray value_str = pretty_inspect(value);

			OnStackString<1> os(value_str);

			CharArray expected_str = inspect(expected);

			raise(context->type_error, value_str + " was given when an object of type " + expected_str + " was expected");
		}
	}
	
	Exception *create_exception(Class *exception_class, const CharArray &message)
	{
		return Collector::allocate<Exception>(exception_class, message.to_string(), backtrace());
	}

	void raise(Class *exception_class, const CharArray &message)
	{
		throw InternalException(create_exception(exception_class, message));
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
				throw InternalException(new (collector) SyntaxError(parser));

			OnStack<3> os(tree_scope, method_name, scope);

			#ifdef MIRB_DEBUG_COMPILER
				DebugPrinter printer;

				context->console_output->puts("Parsing done.\n-----");
				context->console_output->puts(printer.print_node(tree_scope->group));
				context->console_output->puts("-----");
			#endif

			block = Compiler::compile(tree_scope, memory_pool);
		}

		return call_code(block, self, method_name, scope, 0, value_nil, 0, 0);
	}
	
	Method *lookup_module(Module *module, Symbol *name)
	{
		name->assert_valid();

		while(module != nullptr)
		{
			auto result = module->get_method(name);

			if(prelude_likely(result != 0))
			{
				if(prelude_likely(result != value_undef))
					return cast<Method>(result);

				return 0;
			}

			module = module->superclass;
		}

		return 0;
	}
	
	void method_error(Symbol *name, value_t obj, bool in)
	{
		OnStack<1> os(name);

		CharArray obj_str = pretty_inspect(obj);

		raise(context->name_error, "Undefined method '" + name->string + (in ? CharArray("' in ") :  CharArray("' for ")) + obj_str);
	}
	
	Method *lookup_method(Module *module, Symbol *name, value_t obj)
	{
		Method *result = lookup_module(module, name);

		if(!result)
			method_error(name, obj, false);

		return result;
	}
	
	Method *respond_to(value_t obj, Symbol *name)
	{
		return lookup_module(internal_class_of(obj), name);
	}

	Method *lookup(value_t obj, Symbol *name)
	{
		return lookup_method(internal_class_of(obj), name, obj);
	}

	Method *lookup_super(Module *module, Symbol *name)
	{
		Method *result = lookup_module(module->superclass, name);

		if(prelude_unlikely(!result))
		{
			OnStack<1> os(name);

			CharArray module_value = pretty_inspect(module);

			raise(context->name_error, "No superclass method '" + name->string + "' for " + module_value);
		}

		return result;
	}

	bool stack_no_reserve(Frame &frame)
	{
		return ((size_t)&frame <= stack_continue);
	}
	
	void assert_stack_space()
	{
		int local;

		if(prelude_unlikely((size_t)&local <= stack_stop))
			raise(context->system_stack_error, "Stack overflow");
	}

	struct ContextState
	{
		Frame &frame;

		ContextState(Frame &frame) : frame(frame)
		{
			frame.prev = context->frame;
			context->frame = &frame;
		}

		~ContextState()
		{
			context->frame = frame.prev;
		}
	};

	value_t call_frame(Frame &frame)
	{
		if(prelude_unlikely((size_t)&frame <= stack_stop))
			raise(context->system_stack_error, "Stack overflow");

		ContextState frame_state(frame);

		Collector::check();

		if(prelude_likely(!frame.code->block))
		{
			if(prelude_unlikely(frame.argc < frame.code->min_args))
				raise(context->argument_error, "Too few arguments passed to function (" + CharArray::uint(frame.code->min_args) + " required)");

			if(prelude_unlikely(frame.code->max_args != (size_t)-1 && frame.argc > frame.code->max_args))
				raise(context->argument_error, "Too many arguments passed to function (max " + CharArray::uint(frame.code->max_args) + ")");
		}

		return frame.code->executor(frame);
	}

	value_t call_code(Block *code, value_t obj, Symbol *name, Tuple<Module> *scope, Tuple<> *scopes, value_t block, size_t argc, value_t argv[])
	{
		Frame frame;

		frame.code = code;
		frame.obj = obj;
		frame.name = name;
		frame.scope = scope;
		frame.block = block;
		frame.argc = argc;
		frame.argv = argv;
		frame.scopes = scopes;

		return call_frame(frame);
	};
	
	value_t call_argv(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[])
	{
		Method *method = lookup(obj, name);

		return call_argv(method, obj, name, block, argc, argv);
	}
	
	struct OnStackState
	{
		OnStackBlock<false> *os;

		OnStackState(void *stack_memory)
		{
			os = new (stack_memory) OnStackBlock<false>();
		}

		~OnStackState()
		{
			os->~OnStackBlock<false>();
		}
	};

	template<typename F> void on_stack_argv(size_t argc, value_t argv[], F func)
	{
		void *stack_memory = alloca(sizeof(OnStackBlock<false>) + 2 * sizeof(value_t) * argc);

		if(prelude_unlikely(!stack_memory))
			raise(context->runtime_error, "Unable to allocate stack memory");

		OnStackState oss(stack_memory);

		oss.os->size = argc;

		value_t *os_array = (value_t *)(oss.os + 1);

		for(size_t i = 0; i < argc; ++i)
		{
			os_array[i] = (value_t)&os_array[argc + i];
			os_array[argc + i] = argv[i];
		}

		func(&os_array[argc]);
	}
	
	value_t call_argv(Method *method, value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[])
	{
		value_t result;

		on_stack_argv(argc, argv, [&](value_t *on_stack_argv) {
			result = call_code(method->block, obj, name, method->scope, method->scopes, block, argc, on_stack_argv);
		});

		return result;
	}
	
	Proc *get_proc(value_t obj)
	{
		if(prelude_unlikely(obj == value_nil))
			raise(context->local_jump_error, "No block given");

		return raise_cast<Proc>(obj);
	}
	
	value_t yield_argv(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		Proc *proc = get_proc(obj);

		value_t result;

		on_stack_argv(argc, argv, [&](value_t *on_stack_argv) {
			result = Proc::call(proc, block, argc, on_stack_argv);
		});

		return result;
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
	
	Array *cast_array(value_t value)
	{
		auto array = try_cast<Array>(value);

		if(array)
			return array;

		auto method = respond_to(value, "to_a");

		if(method)
			return raise_cast<Array>(call_argv(method, value, Symbol::get("to_a"), value_nil, 0, 0));
		else
		{
			array = new (collector) Array;
			array->vector.push(value);
			return array;
		}
	}
	
	String *cast_string(value_t value)
	{
		auto string = try_cast<String>(value);

		if(string)
			return string;

		auto method = respond_to(value, "to_s");

		if(method)
			return raise_cast<String>(call_argv(method, value, Symbol::get("to_s"), value_nil, 0, 0));
		else
		{
			CharArray obj = pretty_inspect(value);

			raise(context->type_error, "Unable to convert " + obj + " to string");
		}
	}
	
	value_t cast_integer(value_t value)
	{
		if(value->type() == Type::Fixnum)
			return value;

		auto method = respond_to(value, "to_i");

		if(method)
			return raise_cast<Type::Fixnum>(call_argv(method, value, Symbol::get("to_i"), value_nil, 0, 0));
		else
		{
			CharArray obj = pretty_inspect(value);

			raise(context->type_error, "Unable to convert " + obj + " to integer");
		}
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

		context->bootstrap = false;
	}

	void setup_main()
	{
		context->main = Collector::allocate<Object>(context->object_class);

		singleton_method<&main_to_s>(context->main, "to_s");
		singleton_method<Arg::Count, Arg::Values, &main_include>(context->main, "include");
	}
	
	const size_t buffer_zone = 0x1000 * 16;
	
	void initialize_thread_initial()
	{
		mirb_runtime_assert(thread_context == 0);

		size_t stack_start = Platform::stack_start();
		size_t limit = Platform::stack_limit();
		
		mirb_runtime_assert(stack_start > limit);
		mirb_runtime_assert(limit > 3 * buffer_zone);
		
		stack_stop = stack_start - limit + buffer_zone;
		stack_continue = stack_stop + buffer_zone;

		thread_context = new ThreadContext;

		thread_contexts.append(thread_context);
	}
	
	void initialize_thread_later()
	{
		Platform::wrap([&] {
			thread_context->current_directory = File::normalize_path(Platform::cwd());
		});
	}

	void initialize_thread()
	{
		initialize_thread_initial();
		initialize_thread_later();
	}

	void initialize(bool console)
	{
		Number::initialize();
		Collector::initialize();

		context = new Context;

		context->dummy_map = Collector::allocate<ValueMap>();

		initialize_thread_initial();

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
		Bignum::initialize();
		Time::initialize();
		initialize_exceptions();
		
		Collector::enable_interrupts = true;
		
		IO::initialize();
		File::initialize();
		Dir::initialize();
		Regexp::initialize();

		setup_main();

		set_const(context->object_class, Symbol::get("RUBY_ENGINE"), String::get("mirb"));
		set_const(context->object_class, Symbol::get("RUBY_VERSION"), String::get("1.9"));

#ifdef WIN32
		set_const(context->object_class, Symbol::get("RUBY_PLATFORM"), String::get("mirb-winapi"));
#else
		set_const(context->object_class, Symbol::get("RUBY_PLATFORM"), String::get("mirb-posix"));
#endif
		context->loaded_files = Collector::allocate<Array>();

		auto const_global = [&](const CharArray &name, value_t value) -> Global *{
			auto global = new (collector) Global;
			global->value = value;
			global->read_only();
			set_global_object(Symbol::get(name), global);
			return global;
		};
		
		context->loaded_files = Collector::allocate<Array>();
		const_global("$LOADED_FEATURES", context->loaded_files);

		{
			context->load_paths = Collector::allocate<Array>();

			auto global = const_global("$LOAD_PATH", context->load_paths);
			set_global_object(Symbol::get("$:"), global);
			set_global_object(Symbol::get("$-I"), global);
		}

		initialize_thread_later();
		
		context->console_input = Platform::console_stream(Platform::StandardInput);
		context->console_output = Platform::console_stream(Platform::StandardOutput);
		context->console_error = Platform::console_stream(Platform::StandardError);
		
		context->io_in = new (collector) IO(context->console_input, context->io_class, false);
		context->io_out = new (collector) IO(context->console_output, context->io_class, false);
		context->io_err = new (collector) IO(context->console_error, context->io_class, false);
		
		set_global(Symbol::get("$stdin"), context->io_in);
		set_global(Symbol::get("$stdout"), context->io_out);
		set_global(Symbol::get("$stderr"), context->io_err);
		
		set_const(context->object_class, Symbol::get("STDIN"), context->io_in);
		set_const(context->object_class, Symbol::get("STDOUT"), context->io_out);
		set_const(context->object_class, Symbol::get("STDERR"), context->io_err);
				
		mirb_debug(Collector::collect());
	}

	void finalize()
	{
		Exception *e;

		trap_exception(e, [&] {
			for(size_t i = context->at_exits.size(); i-- > 0;)
				Proc::call(cast<Proc>(context->at_exits[i]), value_nil, 0, nullptr);
		});  // TODO: Print if this fails
		
		if(context->console_input)
			delete context->console_input;
		
		if(context->console_output)
			delete context->console_output;

		if(context->console_error)
			delete context->console_error;

		auto thread_context = thread_contexts.first;

		while(thread_context)
		{
			auto next = thread_context->entry.next;

			delete thread_context;

			thread_context = next;
		}

		delete context;

		Platform::finalize();
		Collector::finalize();
		Number::finalize();

	}
};

