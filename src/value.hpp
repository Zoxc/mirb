#pragma once
#include "common.hpp"

#ifdef VALGRIND
	#include <Prelude/LinkedList.hpp>
#endif

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	};
	
	namespace Value
	{
		class Header;
	};
	
	class Collector;
	class FreeBlock;
	class Document;
	class Block;
	class Object;
	class ObjectHeader;
	class VariableBlock;
	template<class T = Value::Header> class Tuple;
	class ValueMap;
	class Module;
	class Class;
	class Symbol;
	class String;
	class Regexp;
	class Method;
	class Array;
	class Hash;
	class Float;
	class Range;
	class StackFrame;
	class Exception;
	class ReturnException;
	class BreakException;
	class NextException;
	class RedoException;
	class SystemStackError;
	class Proc;
	class IO;
	class File;

	/* Format for tagged pointers
	 *
	 *  1 - fixnum
	 * 00 - object reference, only above 0x4 (0x4 would make the value zero if bit 2 and 3 is cleared)
	 * 10 - literal objects
	 */
	 
	/* Bit layout for literal objects. You can test a value by clearing bit 2 and 3, if the result is 0, it's nil or false.
	 *
	 * 0010 - nil
	 * 0110 - false
	 * 1010 - true
	 * 1110 - undef
	 */
	
	namespace Value
	{
		class Header;
	};

	typedef Value::Header *value_t;

	const size_t fixnum_mask = 1;
	
	#define mirb_object_align 4

	const size_t object_ref_mask = mirb_object_align - 1;

	const size_t literal_mask = 0xF;
	const size_t literal_count = (literal_mask + 1);
	
	const value_t value_raise = 0; // Used to indicate that an exception is raised. Also invalid returns from maps.
	
	const size_t value_nil_num = 2;
	const size_t value_false_num = 6;
	const size_t value_true_num = 10;

	const value_t value_nil = (value_t)value_nil_num;
	const value_t value_false = (value_t)value_false_num;
	const value_t value_true = (value_t)value_true_num;
	const value_t value_undef = (value_t)14; // Used to mark words in vectors containing no references.

	namespace Value
	{
		enum Type {
			None,
			FreeBlock,
			InternalValueMap,
			InternalStackFrame,
			InternalTuple,
			InternalValueTuple,
			InternalVariableBlock,
			InternalDocument,
			InternalBlock,
			InternalScope,
			Float,
			Range,
			Fixnum,
			True,
			False,
			Nil,
			Class,
			IClass,
			Module,
			Object,
			Symbol,
			String,
			Regexp,
			Array,
			Hash,
			Method,
			Proc,
			Exception,
			ReturnException,
			BreakException,
			NextException,
			RedoException,
			SystemStackError,
			IO,
			File,
			Types
		};
		
		class Header
		{
			private:
				#ifdef DEBUG
					value_t *mark_data;
					value_t *thread_data;
				#else
					value_t *data;
				#endif

				void setup();
			public:
				Header(const Header &other);
				Header(Type type);
			
				Type get_type();

				static const size_t type_bits = 7;

				static_assert(((1 << type_bits) - 1) >= (size_t)Types, "Too few bits for type field");
				
				static const size_t magic_value;
				static value_t * const list_end;
				
				Type type : type_bits;
				bool marked : 1;
				bool hashed : 1;
				bool flag : 1; // Usable by subclasses

				#ifdef DEBUG
					bool alive : 1;
					size_t block_size;
					size_t magic;
					value_t refs;
				#endif

				typedef value_t *Header::*data_field;
				
				#ifdef DEBUG
					static const data_field mark_list;
					static const data_field thread_list;
				#else
					static const data_field mark_list;
					static const data_field thread_list;
				#endif
				
				#ifdef VALGRIND
					LinkedListEntry<Header> entry;
				#endif

				static value_t dup(value_t obj);

				static const bool finalizer = false;

				size_t hash()
				{
					mirb_runtime_abort("No hash function defined for object");
				}
		};

		template<Type type> struct Immediate:
			public Value::Header
		{
			template<typename F> void mark(F) {}

			size_t hash()
			{
				return hash_number((size_t)this); // Note: First bit is constant
			}
		};
	
		bool object_ref(value_t value);
		
		Mirb::Class *class_of_literal(value_t value);
		
		bool test(value_t value);

		static inline bool immediate(Type type)
		{
			switch(type)
			{
				case Fixnum:
				case True:
				case False:
				case Nil:
					return true;
				default:
					return false;
			}
		}

		Type type(value_t value);
		
		template<template<Type> class T, typename Arg> auto virtual_do(Type type, Arg &&arg) -> typename T<Nil>::Result
		{
			switch(type)
			{
				case InternalValueMap:
					return T<InternalValueMap>::func(std::forward<Arg>(arg));

				case InternalStackFrame:
					return T<InternalStackFrame>::func(std::forward<Arg>(arg));
					
				case InternalTuple:
					return T<InternalTuple>::func(std::forward<Arg>(arg));
					
				case InternalValueTuple:
					return T<InternalValueTuple>::func(std::forward<Arg>(arg));
					
				case InternalVariableBlock:
					return T<InternalVariableBlock>::func(std::forward<Arg>(arg));

				case InternalDocument:
					return T<InternalDocument>::func(std::forward<Arg>(arg));

				case InternalBlock:
					return T<InternalBlock>::func(std::forward<Arg>(arg));

				case InternalScope:
					return T<InternalScope>::func(std::forward<Arg>(arg));
					
				case Float:
					return T<Float>::func(std::forward<Arg>(arg));

				case Range:
					return T<Range>::func(std::forward<Arg>(arg));

				case Fixnum:
					return T<Fixnum>::func(std::forward<Arg>(arg));

				case True:
					return T<True>::func(std::forward<Arg>(arg));

				case False:
					return T<False>::func(std::forward<Arg>(arg));

				case Nil:
					return T<Nil>::func(std::forward<Arg>(arg));
					
				case Class:
					return T<Class>::func(std::forward<Arg>(arg));

				case IClass:
					return T<IClass>::func(std::forward<Arg>(arg));

				case Module:
					return T<Module>::func(std::forward<Arg>(arg));

				case Object:
					return T<Object>::func(std::forward<Arg>(arg));

				case Symbol:
					return T<Symbol>::func(std::forward<Arg>(arg));
					
				case String:
					return T<String>::func(std::forward<Arg>(arg));

				case Regexp:
					return T<Regexp>::func(std::forward<Arg>(arg));

				case Array:
					return T<Array>::func(std::forward<Arg>(arg));

				case Hash:
					return T<Hash>::func(std::forward<Arg>(arg));
					
				case Method:
					return T<Method>::func(std::forward<Arg>(arg));

				case Proc:
					return T<Proc>::func(std::forward<Arg>(arg));

				case Exception:
					return T<Exception>::func(std::forward<Arg>(arg));

				case ReturnException:
					return T<ReturnException>::func(std::forward<Arg>(arg));
					
				case BreakException:
					return T<BreakException>::func(std::forward<Arg>(arg));
					
				case RedoException:
					return T<RedoException>::func(std::forward<Arg>(arg));
					
				case NextException:
					return T<NextException>::func(std::forward<Arg>(arg));

				case SystemStackError:
					return T<SystemStackError>::func(std::forward<Arg>(arg));
					
				case IO:
					return T<IO>::func(std::forward<Arg>(arg));
					
				case File:
					return T<File>::func(std::forward<Arg>(arg));
					
				case FreeBlock:
					return T<FreeBlock>::func(std::forward<Arg>(arg));
					
				case None:
				default:
					mirb_debug_abort("Unknown value type");
			};
		}
		
		template<Type type> struct TypeClass {};
		
		#define mirb_typeclass(tag, type) template<> struct TypeClass<tag> { typedef Mirb::type Class; }
		
		mirb_typeclass(InternalValueMap, ValueMap);
		mirb_typeclass(InternalStackFrame, StackFrame);
		mirb_typeclass(InternalTuple, Tuple<Mirb::Object>);
		mirb_typeclass(InternalValueTuple, Tuple<Value::Header>);
		mirb_typeclass(InternalVariableBlock, VariableBlock);
		mirb_typeclass(InternalDocument, Document);
		mirb_typeclass(InternalBlock, Block);
		mirb_typeclass(InternalScope, Tree::Scope);
		mirb_typeclass(Float, Float);
		mirb_typeclass(Range, Range);
		mirb_typeclass(Fixnum, Value::Immediate<Fixnum>);
		mirb_typeclass(True, Value::Immediate<True>);
		mirb_typeclass(False, Value::Immediate<False>);
		mirb_typeclass(Nil, Value::Immediate<Nil>);
		mirb_typeclass(Class, Class);
		mirb_typeclass(IClass, Class);
		mirb_typeclass(Module, Module);
		mirb_typeclass(Object, Object);
		mirb_typeclass(Symbol, Symbol);
		mirb_typeclass(String, String);
		mirb_typeclass(Regexp, Regexp);
		mirb_typeclass(Array, Array);
		mirb_typeclass(Hash, Hash);
		mirb_typeclass(Method, Method);
		mirb_typeclass(Proc, Proc);
		mirb_typeclass(Exception, Exception);
		mirb_typeclass(ReturnException, ReturnException);
		mirb_typeclass(BreakException, BreakException);
		mirb_typeclass(RedoException, RedoException);
		mirb_typeclass(NextException, NextException);
		mirb_typeclass(SystemStackError, SystemStackError);
		mirb_typeclass(IO, IO);
		mirb_typeclass(File, File);
		mirb_typeclass(FreeBlock, FreeBlock);

		template<class B, class D> struct DerivedFrom
		{
			static const bool value = false;
		};

		#define mirb_derived_from(base, super) template<> struct DerivedFrom<Mirb::base, Mirb::super> { static const bool value = true; }
		
		mirb_derived_from(StackFrame, StackFrame);

		mirb_derived_from(Block, Block);

		mirb_derived_from(Object, Object);
		mirb_derived_from(Object, Module);
		mirb_derived_from(Object, Class);
		mirb_derived_from(Object, Array);
		mirb_derived_from(Object, Hash);
		mirb_derived_from(Object, String);
		mirb_derived_from(Object, Symbol);
		mirb_derived_from(Object, Method);
		mirb_derived_from(Object, Proc);
		mirb_derived_from(Object, Exception);
		mirb_derived_from(Object, ReturnException);
		mirb_derived_from(Object, BreakException);
		mirb_derived_from(Object, IO);
		mirb_derived_from(Object, File);
		mirb_derived_from(Object, Float);
		mirb_derived_from(Object, Regexp);
		mirb_derived_from(Object, Range);
		
		mirb_derived_from(Module, Module);
		mirb_derived_from(Module, Class);

		mirb_derived_from(Class, Class);
		
		mirb_derived_from(String, String);
		
		mirb_derived_from(Float, Float);
		
		mirb_derived_from(Range, Range);
		
		mirb_derived_from(Regexp, Regexp);
		
		mirb_derived_from(Symbol, Symbol);

		mirb_derived_from(Hash, Hash);
		
		mirb_derived_from(Proc, Proc);
		
		mirb_derived_from(Method, Method);
		
		mirb_derived_from(Exception, Exception);
		mirb_derived_from(Exception, ReturnException);
		mirb_derived_from(Exception, BreakException);

		mirb_derived_from(ReturnException, ReturnException);
		mirb_derived_from(ReturnException, BreakException);
		
		mirb_derived_from(BreakException, BreakException);
		
		mirb_derived_from(SystemStackError, SystemStackError);
		
		mirb_derived_from(IO, IO);
		mirb_derived_from(IO, File);

		mirb_derived_from(File, File);

		mirb_derived_from(Array, Array);

		template<class Base> struct OfType
		{
			template<Type type> struct Test
			{
				typedef bool Result;

				static bool func(bool)
				{
					return DerivedFrom<Base, typename TypeClass<type>::Class>::value;
				}
			};
		};
		
		inline void assert_valid_base(value_t obj) prelude_nonnull(1);
		inline void assert_valid_base(value_t obj  prelude_unused)
		{
			mirb_debug(mirb_debug_assert(obj->magic == Header::magic_value));
			mirb_debug_assert(obj->type != None && obj->type != FreeBlock);
		}

		inline void assert_alive(value_t obj) prelude_nonnull(1);
		inline void assert_alive(value_t obj  prelude_unused)
		{
			assert_valid_base(obj);
			mirb_debug(mirb_debug_assert(obj->alive));
		}

		inline void assert_valid(value_t obj) prelude_nonnull(1);
		inline void assert_valid(value_t obj  prelude_unused)
		{
			if(object_ref(obj))
			{
				assert_alive(obj);
				mirb_debug_assert(obj->marked == false);
				mirb_debug_assert((obj->*Header::mark_list) == Value::Header::list_end);
				mirb_debug_assert((obj->*Header::thread_list) == Value::Header::list_end);
			}
		}

		template<class T> bool of_type(value_t value) prelude_nonnull(1);

		template<class T> bool of_type(value_t value)
		{
			Value::assert_valid(value);

			return virtual_do<OfType<T>::template Test>(type(value), true);
		}
		
		template<class T> void verify(T *value prelude_unused)
		{
			mirb_debug_assert(Value::of_type<T>((value_t)value));
		}
		
		bool is_fixnum(value_t value);
		
		size_t hash(value_t value);
		
		void initialize_type_table();
		void initialize_class_table();

		inline value_t from_bool(bool value)
		{
			return value ? value_true : value_false;
		}
	};
	
	template<typename T> T *cast_null(value_t obj)
	{
		mirb_debug_assert(obj == 0 || Value::of_type<T>(obj));
		return reinterpret_cast<T *>(obj);
	}

	template<typename T> T *cast(value_t obj)
	{
		mirb_debug_assert(Value::of_type<T>(obj));
		return reinterpret_cast<T *>(obj);
	}

	template<typename T> T *try_cast(value_t obj)
	{
		if(Value::of_type<T>(obj))
			return reinterpret_cast<T *>(obj);
		else
			return 0;
	}
};
