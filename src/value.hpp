#pragma once
#include "common.hpp"
#include <Prelude/Map.hpp>
#include "gc.hpp"

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	}

	class Document;
	class Block;
	class Object;
	class ObjectHeader;
	class Module;
	class Class;
	class Symbol;
	class String;
	class Array;
	class Exception;
	class ReturnException;
	class BreakException;
	class Proc;

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
	
	typedef ObjectHeader *value_t;

	const size_t fixnum_mask = 1;
	const size_t object_ref_mask = 3;
	
	const size_t literal_mask = 0xF;
	const size_t literal_count = (literal_mask + 1);
	
	const value_t value_raise = 0; // Used to indicate that an exception is raised to native code.
	
	const size_t value_nil_num = 2;
	const size_t value_false_num = 6;
	const size_t value_true_num = 10;

	const value_t value_nil = (value_t)value_nil_num;
	const value_t value_false = (value_t)value_false_num;
	const value_t value_true = (value_t)value_true_num;
	const value_t value_undef = (value_t)14;
	const value_t value_highest = value_undef;

	namespace Value
	{
		enum Type {
			None,
			InternalDocument,
			InternalBlock,
			InternalScope,
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
			Array,
			Proc,
			Exception,
			ReturnException,
			BreakException,
		};

		template<Type type> struct TypeClass
		{
			typedef value_t Class;
		};
		
		template<template<typename,Type> class T, class A, typename Arg> auto virtual_do(Type type, Arg arg) -> decltype(T<A, None>::func(arg))
		{
			switch(type)
			{
				case InternalDocument:
					return T<A, InternalDocument>::func(arg);

				case InternalBlock:
					return T<A, InternalBlock>::func(arg);

				case InternalScope:
					return T<A, InternalScope>::func(arg);

				case Fixnum:
					return T<A, Fixnum>::func(arg);

				case True:
					return T<A, True>::func(arg);

				case False:
					return T<A, False>::func(arg);

				case Nil:
					return T<A, Nil>::func(arg);
					
				case Class:
					return T<A, Class>::func(arg);

				case IClass:
					return T<A, IClass>::func(arg);

				case Module:
					return T<A, Module>::func(arg);

				case Object:
					return T<A, Object>::func(arg);

				case Symbol:
					return T<A, Symbol>::func(arg);

				case String:
					return T<A, String>::func(arg);

				case Array:
					return T<A, Array>::func(arg);

				case Proc:
					return T<A, Proc>::func(arg);

				case Exception:
					return T<A, Exception>::func(arg);

				case ReturnException:
					return T<A, ReturnException>::func(arg);

				case BreakException:
					return T<A, BreakException>::func(arg);

				case None:
				default:
					mirb_debug_abort("Unknown value type");
			};
		}

		template<> struct TypeClass<InternalDocument> { typedef Mirb::Document Class; };
		template<> struct TypeClass<InternalBlock> { typedef Mirb::Block Class; };
		template<> struct TypeClass<InternalScope> { typedef Mirb::Tree::Scope Class; };
		template<> struct TypeClass<Class> { typedef Mirb::Class Class; };
		template<> struct TypeClass<IClass> { typedef Mirb::Class Class; };
		template<> struct TypeClass<Module> { typedef Mirb::Module Class; };
		template<> struct TypeClass<Object> { typedef Mirb::Object Class; };
		template<> struct TypeClass<Symbol> { typedef Mirb::Symbol Class; };
		template<> struct TypeClass<String> { typedef Mirb::String Class; };
		template<> struct TypeClass<Array> { typedef Mirb::Array Class; };
		template<> struct TypeClass<Proc> { typedef Mirb::Proc Class; };
		template<> struct TypeClass<Exception> { typedef Mirb::Exception Class; };
		template<> struct TypeClass<ReturnException> { typedef Mirb::ReturnException Class; };
		template<> struct TypeClass<BreakException> { typedef Mirb::BreakException Class; };

		bool object_ref(value_t value);
		
		value_t class_of_literal(value_t value);

		bool test(value_t value);

		template<class T> struct OfType
		{
			template<class A, Type type> struct Test
			{
				bool func(bool dummy)
				{
					return std::is_base_of<typename TypeClass<type>::Class, T>::value;
				}
			};
		};

		Type type(value_t value);
		
		template<class T> bool of_type(value_t value)
		{
			return virtual_do<OfType<T>::template Test, bool, bool>(type(value), true);
		}
		
		void initialize();
	};
	
	union auto_cast
	{
		value_t value;
			
		auto_cast(value_t value) : value(value) {}

		auto_cast(bool value)
		{
			this->value = value ? value_true : value_false;
		}
		
		auto_cast(Object *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Object>(value));
		}

		auto_cast(Module *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Module>(value));
		}

		auto_cast(Class *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Class>(value));
		}
		
		auto_cast(Symbol *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Symbol>(value));
		}
		
		auto_cast(String *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<String>(value));
		}

		auto_cast(Array *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Array>(value));
		}
		
		auto_cast(Exception *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Exception>(value));
		}

		auto_cast(Proc *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Proc>(value));
		}

		operator value_t() { return value; }
		
		operator Object *()
		{
			mirb_debug_assert(Value::of_type<Object>(value));
			return (Object *)value;
		}

		operator Module *()
		{
			mirb_debug_assert(Value::of_type<Module>(value));
			return (Module *)value;
		}

		operator Class *()
		{
			mirb_debug_assert(Value::of_type<Class>(value));
			return (Class *)value;
		}

		operator Symbol *()
		{
			mirb_debug_assert(Value::of_type<Symbol>(value));
			return (Symbol *)value;
		}

		operator String *()
		{
			mirb_debug_assert(Value::of_type<String>(value));
			return (String *)value;
		}
		
		operator Array *()
		{
			mirb_debug_assert(Value::of_type<Array>(value));
			return (Array *)value;
		}
		
		operator Exception *()
		{
			mirb_debug_assert(Value::of_type<Exception>(value));
			return (Exception *)value;
		}
		
		operator Proc *()
		{
			mirb_debug_assert(Value::of_type<Proc>(value));
			return (Proc *)value;
		}
	};

	template<typename T> T *cast(value_t obj)
	{
		mirb_debug_assert(Value::of_type<T>(obj));
		return (T *)obj;
	}
	
	class ValueMapFunctions:
		public MapFunctions<value_t, value_t>
	{
		public:
			static value_t invalid_value()
			{
				return value_undef;
			}
	};

	typedef Map<value_t, value_t, GC, ValueMapFunctions> ValueMap;
};

