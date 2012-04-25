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
	class Tuple;
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
			InternalTuple,
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

		bool object_ref(value_t value);
		
		value_t class_of_literal(value_t value);

		bool test(value_t value);

		Type type(value_t value);
		
		template<template<Type> class T, typename Arg> auto virtual_do(Type type, Arg &&arg) -> typename T<None>::Result
		{
			switch(type)
			{
				case InternalTuple:
					return T<InternalTuple>::func(std::forward<Arg>(arg));

				case InternalDocument:
					return T<InternalDocument>::func(std::forward<Arg>(arg));

				case InternalBlock:
					return T<InternalBlock>::func(std::forward<Arg>(arg));

				case InternalScope:
					return T<InternalScope>::func(std::forward<Arg>(arg));

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

				case Array:
					return T<Array>::func(std::forward<Arg>(arg));

				case Proc:
					return T<Proc>::func(std::forward<Arg>(arg));

				case Exception:
					return T<Exception>::func(std::forward<Arg>(arg));

				case ReturnException:
					return T<ReturnException>::func(std::forward<Arg>(arg));

				case BreakException:
					return T<BreakException>::func(std::forward<Arg>(arg));

				case None:
				default:
					mirb_debug_abort("Unknown value type");
			};
		}
		
		template<Type type> struct TypeClass {};
		
		#define mirb_typeclass(tag, type) template<> struct TypeClass<tag> { typedef Mirb::type Class; }
		
		mirb_typeclass(InternalTuple, Tuple);
		mirb_typeclass(InternalDocument, Document);
		mirb_typeclass(InternalBlock, Block);
		mirb_typeclass(InternalScope, Tree::Scope);
		mirb_typeclass(Fixnum, value_t);
		mirb_typeclass(True, value_t);
		mirb_typeclass(False, value_t);
		mirb_typeclass(Nil, value_t);
		mirb_typeclass(Class, Class);
		mirb_typeclass(IClass, Class);
		mirb_typeclass(Module, Module);
		mirb_typeclass(Object, Object);
		mirb_typeclass(Symbol, Symbol);
		mirb_typeclass(String, String);
		mirb_typeclass(Array, Array);
		mirb_typeclass(Proc, Proc);
		mirb_typeclass(Exception, Exception);
		mirb_typeclass(ReturnException, ReturnException);
		mirb_typeclass(BreakException, BreakException);

		template<class B, class D> struct DerivedFrom
		{
			static const bool value = false;
		};

		#define mirb_derived_from(base, super) template<> struct DerivedFrom<Mirb::base, Mirb::super> { static const bool value = true; }
		
		mirb_derived_from(Object, Object);
		mirb_derived_from(Object, Module);
		mirb_derived_from(Object, Class);
		mirb_derived_from(Object, Array);
		mirb_derived_from(Object, String);
		mirb_derived_from(Object, Symbol);
		mirb_derived_from(Object, Proc);
		mirb_derived_from(Object, Exception);
		mirb_derived_from(Object, ReturnException);
		mirb_derived_from(Object, BreakException);
		
		mirb_derived_from(Module, Module);
		mirb_derived_from(Module, Class);

		mirb_derived_from(Class, Class);
		
		mirb_derived_from(String, String);
		
		mirb_derived_from(Symbol, Symbol);
		
		mirb_derived_from(Proc, Proc);
		
		mirb_derived_from(Exception, Exception);
		mirb_derived_from(Exception, ReturnException);
		mirb_derived_from(Exception, BreakException);

		mirb_derived_from(ReturnException, ReturnException);
		mirb_derived_from(ReturnException, BreakException);

		mirb_derived_from(BreakException, BreakException);
		
		mirb_derived_from(Array, Array);

		template<class Base> struct OfType
		{
			template<Type type> struct Test
			{
				typedef bool Result;

				static bool func(bool dummy)
				{
					return DerivedFrom<Base, typename TypeClass<type>::Class>::value;
				}
			};
		};

		template<class T> bool of_type(value_t value)
		{
			static_assert(std::is_base_of<Mirb::Object, T>::value, "T must be a Object");

			return virtual_do<OfType<T>::template Test, bool>(type(value), true);
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
		
		template<class T> auto_cast(T *obj) : value((value_t)obj)
		{
			static_assert(std::is_base_of<Mirb::Object, T>::value, "T must be a Object");

			mirb_debug_assert(Value::of_type<T>(value));
		}

		operator value_t() { return value; }
		
		template<class T> operator T *()
		{
			static_assert(std::is_base_of<Mirb::Object, T>::value, "T must be a Object");

			mirb_debug_assert(Value::of_type<T>(value));
			return (T *)value;
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
