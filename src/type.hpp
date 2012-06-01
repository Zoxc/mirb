#pragma once
#include "common.hpp"

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	};
	
	class Global;
	class Collector;
	class FreeBlock;
	class Document;
	class Block;
	class Object;
	class ObjectHeader;
	class VariableBlock;
	class Value;
	template<class T = Value> class Tuple;
	class ValueMap;
	class Module;
	class Class;
	class Symbol;
	class String;
	class Regexp;
	class Bignum;
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
	class SyntaxError;
	class SystemExit;
	class Proc;
	class IO;
	class ImmediateFixnum;
	class ImmediateTrue;
	class ImmediateFalse;
	class ImmediateNil;

	namespace Type
	{
		enum Enum {
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
			InternalGlobal,
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
			Bignum,
			Array,
			Hash,
			Method,
			Proc,
			Exception,
			ReturnException,
			BreakException,
			NextException,
			RedoException,
			SystemExit,
			SystemStackError,
			SyntaxError,
			IO,
			Types
		};

		extern std::string names[Types];
		
		template<template<Enum> class T, typename Arg> auto action(Enum type, Arg &&arg) -> typename T<Nil>::Result
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
					
				case InternalGlobal:
					return T<InternalGlobal>::func(std::forward<Arg>(arg));
					
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

				case Bignum:
					return T<Bignum>::func(std::forward<Arg>(arg));

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
					
				case SystemExit:
					return T<SystemExit>::func(std::forward<Arg>(arg));
					
				case SystemStackError:
					return T<SystemStackError>::func(std::forward<Arg>(arg));
					
				case SyntaxError:
					return T<SyntaxError>::func(std::forward<Arg>(arg));
					
				case IO:
					return T<IO>::func(std::forward<Arg>(arg));
					
				case FreeBlock:
					return T<FreeBlock>::func(std::forward<Arg>(arg));
					
				case None:
				default:
					mirb_debug_abort("Unknown value type");
			};
		}
		
		template<Type::Enum type> struct ToClass {};
		template<class T> struct ToTag {};	

		#define mirb_only_typeclass(tag, type) template<> struct ToClass<Type::tag> { typedef Mirb::type Class; };
		#define mirb_only_typetag(tag, type) template<> struct ToTag<Mirb::type> { static const Type::Enum value = Type::tag; };
		#define mirb_typeclass(tag, type) mirb_only_typeclass(tag, type); mirb_only_typetag(tag, type);
		
		mirb_typeclass(InternalValueMap, ValueMap);
		mirb_typeclass(InternalStackFrame, StackFrame);
		mirb_typeclass(InternalTuple, Tuple<Mirb::Object>);
		mirb_typeclass(InternalValueTuple, Tuple<Mirb::Value>);
		mirb_typeclass(InternalVariableBlock, VariableBlock);
		mirb_typeclass(InternalDocument, Document);
		mirb_typeclass(InternalBlock, Block);
		mirb_typeclass(InternalScope, Tree::Scope);
		mirb_typeclass(InternalGlobal, Global);
		mirb_typeclass(Float, Float);
		mirb_typeclass(Range, Range);
		mirb_typeclass(Fixnum, ImmediateFixnum);
		mirb_typeclass(True, ImmediateTrue);
		mirb_typeclass(False, ImmediateFalse);
		mirb_typeclass(Nil, ImmediateNil);
		mirb_typeclass(Class, Class);
		mirb_only_typeclass(IClass, Class);
		mirb_typeclass(Module, Module);
		mirb_typeclass(Object, Object);
		mirb_typeclass(Symbol, Symbol);
		mirb_typeclass(String, String);
		mirb_typeclass(Regexp, Regexp);
		mirb_typeclass(Bignum, Bignum);
		mirb_typeclass(Array, Array);
		mirb_typeclass(Hash, Hash);
		mirb_typeclass(Method, Method);
		mirb_typeclass(Proc, Proc);
		mirb_typeclass(Exception, Exception);
		mirb_typeclass(ReturnException, ReturnException);
		mirb_typeclass(BreakException, BreakException);
		mirb_typeclass(RedoException, RedoException);
		mirb_typeclass(NextException, NextException);
		mirb_typeclass(SystemExit, SystemExit);
		mirb_typeclass(SystemStackError, SystemStackError);
		mirb_typeclass(SyntaxError, SyntaxError);
		mirb_typeclass(IO, IO);
		mirb_typeclass(FreeBlock, FreeBlock);

		template<class B, class D> struct DerivedFrom
		{
			static const bool value = false;
		};

		#define mirb_derived_from(base, super) template<> struct DerivedFrom<Mirb::base, Mirb::super> { static const bool value = true; }
		
		mirb_derived_from(StackFrame, StackFrame);
		mirb_derived_from(Global, Global);

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
		mirb_derived_from(Object, SystemExit);
		mirb_derived_from(Object, SystemStackError);
		mirb_derived_from(Object, SyntaxError);
		mirb_derived_from(Object, IO);
		mirb_derived_from(Object, Float);
		mirb_derived_from(Object, Regexp);
		mirb_derived_from(Object, Range);
		mirb_derived_from(Object, Bignum);
		
		mirb_derived_from(Bignum, Bignum);

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
		mirb_derived_from(Exception, SystemExit);
		mirb_derived_from(Exception, SystemStackError);
		mirb_derived_from(Exception, SyntaxError);

		mirb_derived_from(ReturnException, ReturnException);
		mirb_derived_from(ReturnException, BreakException);
		
		mirb_derived_from(BreakException, BreakException);
		
		mirb_derived_from(SystemStackError, SystemStackError);
		
		mirb_derived_from(SystemExit, SystemExit);
		
		mirb_derived_from(SyntaxError, SyntaxError);
		
		mirb_derived_from(IO, IO);

		mirb_derived_from(Array, Array);
	};
};
