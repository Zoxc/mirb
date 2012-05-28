#pragma once
#include "../common.hpp"
#include "../value.hpp"
#include "../block.hpp"
#include "../lexer/lexeme.hpp"

namespace Mirb
{
	class Symbol;
	class Block;
	
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class BasicBlock;
		
		struct Opcode
		{
			enum Type
			{
				Move,
				LoadFixnum,
				LoadFloat,
				LoadTrue,
				LoadFalse,
				LoadNil,
				LoadObject,
				LoadArg,
				LoadArgFloat,
				LoadArrayArg,
				LoadArgBranch,
				LoadSymbol,
				Assign,
				AssignArray,
				Push,
				PushArray,
				AssertBlock,
				Closure,
				Class,
				SingletonClass,
				Module,
				Method,
				SingletonMethod,
				Alias,
				Call,
				Super,
				VariadicCall,
				VariadicSuper,
				Lookup,
				Self,
				Block,
				CreateHeap,
				GetHeapVar,
				SetHeapVar,
				GetIVar,
				SetIVar,
				GetGlobal,
				SetGlobal,
				GetScopedConst,
				SetScopedConst,
				GetConst,
				SetConst,
				BranchIf,
				BranchUnless,
				Branch,
				Return,
				Handler,
				UnwindEnsure,
				UnwindFilter,
				UnwindReturn,
				UnwindBreak,
				UnwindRedo,
				UnwindNext,
				Array,
				Hash,
				String,
				Regexp,
				Range,
				Interpolate
			};

			#define MIRB_OPCODES \
				&&OpMove, \
				&&OpLoadFixnum, \
				&&OpLoadFloat, \
				&&OpLoadTrue, \
				&&OpLoadFalse, \
				&&OpLoadNil, \
				&&OpLoadObject, \
				&&OpLoadArg, \
				&&OpLoadArgFloat, \
				&&OpLoadArrayArg, \
				&&OpLoadArgBranch, \
				&&OpLoadSymbol, \
				&&OpAssign, \
				&&OpAssignArray, \
				&&OpPush, \
				&&OpPushArray, \
				&&OpAssertBlock, \
				&&OpClosure, \
				&&OpClass, \
				&&OpSingletonClass, \
				&&OpModule, \
				&&OpMethod, \
				&&OpSingletonMethod, \
				&&OpAlias, \
				&&OpCall, \
				&&OpSuper, \
				&&OpVariadicCall, \
				&&OpVariadicSuper, \
				&&OpLookup, \
				&&OpSelf, \
				&&OpBlock, \
				&&OpCreateHeap, \
				&&OpGetHeapVar, \
				&&OpSetHeapVar, \
				&&OpGetIVar, \
				&&OpSetIVar, \
				&&OpGetGlobal, \
				&&OpSetGlobal, \
				&&OpGetScopedConst, \
				&&OpSetScopedConst, \
				&&OpGetConst, \
				&&OpSetConst, \
				&&OpBranchIf, \
				&&OpBranchUnless, \
				&&OpBranch, \
				&&OpReturn, \
				&&OpHandler, \
				&&OpUnwindEnsure, \
				&&OpUnwindFilter, \
				&&OpUnwindReturn, \
				&&OpUnwindBreak, \
				&&OpUnwindRedo, \
				&&OpUnwindNext, \
				&&OpArray, \
				&&OpHash, \
				&&OpString, \
				&&OpRegexp, \
				&&OpRange, \
				&&OpInterpolate

			Type op;

			Opcode(Type op) : op(op) {}
		};

		template<Opcode::Type type> struct OpcodeWrapper:
			public Opcode
		{
			OpcodeWrapper() : Opcode(type) {}
		};
		
		struct BranchOpcode:
			public Opcode
		{
			size_t pos;

			BranchOpcode(Opcode::Type type) : Opcode(type) {}
		};
		
		struct MoveOp:
			public OpcodeWrapper<Opcode::Move>
		{
			var_t dst;
			var_t src;

			MoveOp(var_t dst, var_t src) : dst(dst), src(src) {}
		};
		
		struct LoadFloatOp:
			public OpcodeWrapper<Opcode::LoadFloat>
		{
			var_t var;
			double value;

			LoadFloatOp(var_t var, double value) : var(var), value(value) {}
		};
		
		struct LoadFixnumOp:
			public OpcodeWrapper<Opcode::LoadFixnum>
		{
			var_t var;
			value_t num;

			LoadFixnumOp(var_t var, value_t num) : var(var), num(num) {}
		};
		
		struct LoadTrueOp:
			public OpcodeWrapper<Opcode::LoadTrue>
		{
			var_t var;

			LoadTrueOp(var_t var) : var(var) {}
		};
		
		struct LoadFalseOp:
			public OpcodeWrapper<Opcode::LoadFalse>
		{
			var_t var;

			LoadFalseOp(var_t var) : var(var) {}
		};
		
		struct LoadNilOp:
			public OpcodeWrapper<Opcode::LoadNil>
		{
			var_t var;

			LoadNilOp(var_t var) : var(var) {}
		};
		
		struct LoadObjectOp:
			public OpcodeWrapper<Opcode::LoadObject>
		{
			var_t var;

			LoadObjectOp(var_t var) : var(var) {}
		};
		
		struct LoadArgOp:
			public OpcodeWrapper<Opcode::LoadArg>
		{
			var_t var;
			size_t arg;

			LoadArgOp(var_t var, size_t arg) : var(var), arg(arg) {}
		};
		
		struct LoadArgFloatOp:
			public OpcodeWrapper<Opcode::LoadArgFloat>
		{
			var_t var;
			size_t prev_reg;
			size_t prev_def;

			LoadArgFloatOp(var_t var, size_t prev_reg, size_t prev_def) : var(var), prev_reg(prev_reg), prev_def(prev_def) {}
		};
		
		struct LoadArrayArgOp:
			public OpcodeWrapper<Opcode::LoadArrayArg>
		{
			var_t var;
			size_t from_arg;

			LoadArrayArgOp(var_t var, size_t from_arg) : var(var), from_arg(from_arg) {}
		};
		
		struct LoadArgBranchOp:
			public BranchOpcode
		{
			var_t var;
			size_t arg;
			size_t req_args;

			LoadArgBranchOp(var_t var, size_t arg, size_t req_args) : BranchOpcode(LoadArgBranch), var(var), arg(arg), req_args(req_args) {}
		};
		
		struct LoadSymbolOp:
			public OpcodeWrapper<Opcode::LoadSymbol>
		{
			var_t var;
			Symbol *symbol;

			LoadSymbolOp(var_t var, Symbol *symbol) : var(var), symbol(symbol) {}
		};
		
		struct AssignOp:
			public OpcodeWrapper<Opcode::Assign>
		{
			var_t var;
			var_t array;
			intptr_t index;
			size_t size;

			AssignOp(var_t var, var_t array, int index, size_t size) : var(var), array(array), index(index), size(size) {}
		};
		
		struct AssignArrayOp:
			public OpcodeWrapper<Opcode::AssignArray>
		{
			var_t var;
			var_t array;
			size_t index;
			size_t size;

			AssignArrayOp(var_t var, var_t array, size_t index, size_t size) : var(var), array(array), index(index), size(size) {}
		};
		
		struct PushOp:
			public OpcodeWrapper<Opcode::Push>
		{
			var_t array;
			var_t value;

			PushOp(var_t array, var_t value) : array(array), value(value) {}
		};
		
		struct PushArrayOp:
			public OpcodeWrapper<Opcode::PushArray>
		{
			var_t into;
			var_t from;

			PushArrayOp(var_t into, var_t from) : into(into), from(from) {}
		};
		
		struct AssertBlockOp:
			public OpcodeWrapper<Opcode::AssertBlock>
		{
			var_t var;

			AssertBlockOp(var_t var) : var(var) {}
		};
		
		struct ClosureOp:
			public OpcodeWrapper<Opcode::Closure>
		{
			var_t var;
			Mirb::Block *block;
			size_t argc;
			var_t argv;
			
			ClosureOp(var_t var, Mirb::Block *block, size_t argc, var_t argv) : var(var), block(block), argc(argc), argv(argv) {}
		};
		
		struct ClassOp:
			public OpcodeWrapper<Opcode::Class>
		{
			var_t var;
			var_t scope;
			Symbol *name;
			var_t super;
			Mirb::Block *block;
			
			ClassOp(var_t var, var_t scope, Symbol *name, var_t super, Mirb::Block *block) : var(var), scope(scope), name(name), super(super), block(block) {}
		};
		
		struct SingletonClassOp:
			public OpcodeWrapper<Opcode::SingletonClass>
		{
			var_t var;
			var_t singleton;
			Mirb::Block *block;
			
			SingletonClassOp(var_t var, var_t singleton, Mirb::Block *block) : var(var), singleton(singleton), block(block) {}
		};
		
		struct ModuleOp:
			public OpcodeWrapper<Opcode::Module>
		{
			var_t var;
			var_t scope;
			Symbol *name;
			Mirb::Block *block;
			
			ModuleOp(var_t var, var_t scope, Symbol *name, Mirb::Block *block) : var(var), scope(scope), name(name), block(block) {}
		};
		
		struct MethodOp:
			public OpcodeWrapper<Opcode::Method>
		{
			Symbol *name;
			Mirb::Block *block;
			
			MethodOp(Symbol *name, Mirb::Block *block) : name(name), block(block) {}
		};
		
		struct SingletonMethodOp:
			public OpcodeWrapper<Opcode::SingletonMethod>
		{
			var_t singleton;
			Symbol *name;
			Mirb::Block *block;
			
			SingletonMethodOp(var_t singleton, Symbol *name, Mirb::Block *block) : singleton(singleton), name(name), block(block) {}
		};
		
		struct AliasOp:
			public OpcodeWrapper<Opcode::Alias>
		{
			var_t new_name;
			var_t old_name;
			
			AliasOp(var_t new_name, var_t old_name) : new_name(new_name), old_name(old_name) {}
		};
		
		struct CallOp:
			public OpcodeWrapper<Opcode::Call>
		{
			var_t var;
			var_t obj;
			Symbol *method;
			var_t block_var;
			size_t argc;
			var_t argv;

			CallOp(var_t var, var_t obj, Symbol *method, var_t block_var, size_t argc, var_t argv) : var(var), obj(obj), method(method), block_var(block_var), argc(argc), argv(argv)
			{
				Value::assert_valid(method);
			}
		};
		
		struct SuperOp:
			public OpcodeWrapper<Opcode::Super>
		{
			var_t var;
			var_t block_var;
			size_t argc;
			var_t argv;
			
			SuperOp(var_t var, var_t block_var, size_t argc, var_t argv) : var(var), block_var(block_var), argc(argc), argv(argv) {}
		};
		
		struct VariadicCallOp:
			public OpcodeWrapper<Opcode::VariadicCall>
		{
			var_t var;
			var_t obj;
			Symbol *method;
			var_t block_var;
			var_t argv;

			VariadicCallOp(var_t var, var_t obj, Symbol *method, var_t block_var, var_t argv) : var(var), obj(obj), method(method), block_var(block_var), argv(argv)
			{
				Value::assert_valid(method);
			}
		};
		
		struct VariadicSuperOp:
			public OpcodeWrapper<Opcode::VariadicSuper>
		{
			var_t var;
			var_t block_var;
			var_t argv;
			
			VariadicSuperOp(var_t var, var_t block_var, var_t argv) : var(var), block_var(block_var), argv(argv) {}
		};
		
		struct LookupOp:
			public OpcodeWrapper<Opcode::Lookup>
		{
			var_t var;
			size_t index;
			
			LookupOp(var_t var, size_t index) : var(var), index(index) {}
		};
		
		struct SelfOp:
			public OpcodeWrapper<Opcode::Self>
		{
			var_t var;

			SelfOp(var_t var) : var(var) {}
		};
		
		struct BlockOp:
			public OpcodeWrapper<Opcode::Block>
		{
			var_t var;

			BlockOp(var_t var) : var(var) {}
		};
		
		struct CreateHeapOp:
			public OpcodeWrapper<Opcode::CreateHeap>
		{
			var_t var;
			size_t vars;
			
			CreateHeapOp(var_t var, size_t vars) : var(var), vars(vars) {}
		};
		
		struct GetHeapVarOp:
			public OpcodeWrapper<Opcode::GetHeapVar>
		{
			var_t var;
			var_t heap;
			var_t index;

			GetHeapVarOp(var_t var, var_t heap, var_t index) : var(var), heap(heap), index(index) {}
		};
		
		struct SetHeapVarOp:
			public OpcodeWrapper<Opcode::SetHeapVar>
		{
			var_t heap;
			var_t index;
			var_t var;

			SetHeapVarOp(var_t heap, var_t index, var_t var) : heap(heap), index(index), var(var) {}
		};
		
		struct GetIVarOp:
			public OpcodeWrapper<Opcode::GetIVar>
		{
			var_t var;
			Symbol *name;

			GetIVarOp(var_t var, Symbol *name) : var(var), name(name) {}
		};
		
		struct SetIVarOp:
			public OpcodeWrapper<Opcode::SetIVar>
		{
			Symbol *name;
			var_t var;
			
			SetIVarOp(Symbol *name, var_t var) : name(name), var(var) {}
		};
		
		struct GetGlobalOp:
			public OpcodeWrapper<Opcode::GetGlobal>
		{
			var_t var;
			Symbol *name;

			GetGlobalOp(var_t var, Symbol *name) : var(var), name(name) {}
		};
		
		struct SetGlobalOp:
			public OpcodeWrapper<Opcode::SetGlobal>
		{
			Symbol *name;
			var_t var;
			
			SetGlobalOp(Symbol *name, var_t var) : name(name), var(var) {}
		};
		
		struct GetScopedConstOp:
			public OpcodeWrapper<Opcode::GetScopedConst>
		{
			var_t var;
			var_t obj;
			Symbol *name;
			
			GetScopedConstOp(var_t var, var_t obj, Symbol *name) : var(var), obj(obj), name(name) {}
		};
		
		struct SetScopedConstOp:
			public OpcodeWrapper<Opcode::SetScopedConst>
		{
			var_t obj;
			Symbol *name;
			var_t var;
			
			SetScopedConstOp(var_t obj, Symbol *name, var_t var) : obj(obj), name(name), var(var) {}
		};
		
		struct GetConstOp:
			public OpcodeWrapper<Opcode::GetConst>
		{
			var_t var;
			Symbol *name;
			
			GetConstOp(var_t var, Symbol *name) : var(var), name(name) {}
		};
		
		struct SetConstOp:
			public OpcodeWrapper<Opcode::SetConst>
		{
			Symbol *name;
			var_t var;
			
			SetConstOp(Symbol *name, var_t var) : name(name), var(var) {}
		};
		
		struct BranchIfOp:
			public BranchOpcode
		{
			var_t var;

			BranchIfOp(var_t var) : BranchOpcode(BranchIf), var(var) {}
		};
		
		struct BranchUnlessOp:
			public BranchOpcode
		{
			var_t var;

			BranchUnlessOp(var_t var) : BranchOpcode(BranchUnless), var(var) {}
		};
		
		struct BranchOp:
			public BranchOpcode
		{
			BranchOp() : BranchOpcode(Branch) {}
		};
		
		struct ReturnOp:
			public OpcodeWrapper<Opcode::Return>
		{
			var_t var;
			
			ReturnOp(var_t var) : var(var) {}
		};
		
		struct HandlerOp:
			public OpcodeWrapper<Opcode::Handler>
		{
			ExceptionBlock *block;
			
			HandlerOp(ExceptionBlock *block) : block(block) {}
		};
		
		struct UnwindEnsureOp:
			public OpcodeWrapper<Opcode::UnwindEnsure>
		{
		};
		
		struct UnwindFilterOp:
			public OpcodeWrapper<Opcode::UnwindFilter>
		{
		};
		
		struct UnwindReturnOp:
			public OpcodeWrapper<Opcode::UnwindReturn>
		{
			var_t var;
			Mirb::Block *code;

			UnwindReturnOp(var_t var, Mirb::Block *code) : var(var), code(code) {}
		};
		
		struct UnwindNextOp:
			public OpcodeWrapper<Opcode::UnwindNext>
		{
			var_t var;

			UnwindNextOp(var_t var) : var(var) {}
		};
		
		struct UnwindRedoOp:
			public BranchOpcode
		{
			UnwindRedoOp() : BranchOpcode(UnwindRedo) {}
		};
		
		struct UnwindBreakOp:
			public OpcodeWrapper<Opcode::UnwindBreak>
		{
			var_t var;
			Mirb::Block *code;
			var_t parent_dst;

			UnwindBreakOp(var_t var, Mirb::Block *code, var_t parent_dst) : var(var), code(code), parent_dst(parent_dst) {}
		};
		
		struct ArrayOp:
			public OpcodeWrapper<Opcode::Array>
		{
			var_t var;
			size_t argc;
			var_t argv;

			ArrayOp(var_t var, size_t argc, var_t argv) : var(var), argc(argc), argv(argv) {}
		};
		
		struct HashOp:
			public OpcodeWrapper<Opcode::Hash>
		{
			var_t var;
			size_t argc;
			var_t argv;

			HashOp(var_t var, size_t argc, var_t argv) : var(var), argc(argc), argv(argv) {}
		};
		
		struct StringOp:
			public OpcodeWrapper<Opcode::String>
		{
			var_t var;
			const char_t *str;
			size_t size;

			StringOp(var_t var, const char_t *str, size_t size) : var(var), str(str), size(size) {}
		};
		
		struct RegexpOp:
			public OpcodeWrapper<Opcode::Regexp>
		{
			var_t var;
			const char_t *str;
			size_t size;

			RegexpOp(var_t var, const char_t *str, size_t size) : var(var), str(str), size(size) {}
		};
		
		struct RangeOp:
			public OpcodeWrapper<Opcode::Range>
		{
			var_t var;
			var_t low;
			var_t high;
			bool exclusive;

			RangeOp(var_t var, var_t low, var_t high, bool exclusive) : var(var), low(low), high(high), exclusive(exclusive) {}
		};
		
		struct InterpolateOp:
			public OpcodeWrapper<Opcode::Interpolate>
		{
			var_t var;
			size_t argc;
			var_t argv;
			Value::Type result;

			InterpolateOp(var_t var, size_t argc, var_t argv, Value::Type result) : var(var), argc(argc), argv(argv), result(result) {}
		};
	};
};
