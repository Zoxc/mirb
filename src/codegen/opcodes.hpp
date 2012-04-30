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
				LoadTrue,
				LoadFalse,
				LoadNil,
				LoadObject,
				LoadArg,
				Closure,
				Class,
				Module,
				Method,
				Call,
				Super,
				Lookup,
				Self,
				Block,
				CreateHeap,
				GetHeapVar,
				SetHeapVar,
				GetIVar,
				SetIVar,
				GetConst,
				SetConst,
				BranchIf,
				BranchIfZero,
				BranchUnless,
				BranchUnlessZero,
				Branch,
				Return,
				Handler,
				Unwind,
				UnwindReturn,
				UnwindBreak,
				UnwindRedo,
				UnwindNext,
				Array,
				String,
				Interpolate
			};

			#define MIRB_OPCODES \
				&&OpMove, \
				&&OpLoadFixnum, \
				&&OpLoadTrue, \
				&&OpLoadFalse, \
				&&OpLoadNil, \
				&&OpLoadObject, \
				&&OpLoadArg, \
				&&OpClosure, \
				&&OpClass, \
				&&OpModule, \
				&&OpMethod, \
				&&OpCall, \
				&&OpSuper, \
				&&OpLookup, \
				&&OpSelf, \
				&&OpBlock, \
				&&OpCreateHeap, \
				&&OpGetHeapVar, \
				&&OpSetHeapVar, \
				&&OpGetIVar, \
				&&OpSetIVar, \
				&&OpGetConst, \
				&&OpSetConst, \
				&&OpBranchIf, \
				&&OpBranchIfZero, \
				&&OpBranchUnless, \
				&&OpBranchUnlessZero, \
				&&OpBranch, \
				&&OpReturn, \
				&&OpHandler, \
				&&OpUnwind, \
				&&OpUnwindReturn, \
				&&OpUnwindBreak, \
				&&OpUnwindRedo, \
				&&OpUnwindNext, \
				&&OpArray, \
				&&OpString, \
				&&OpInterpolate

			Type op;

			Opcode(Type op) : op(op) {}
		};

		template<Opcode::Type type> struct OpcodeWrapper:
			public Opcode
		{
			OpcodeWrapper() : Opcode(type) {}
		};
		
		struct MoveOp:
			public OpcodeWrapper<Opcode::Move>
		{
			var_t dst;
			var_t src;

			MoveOp(var_t dst, var_t src) : dst(dst), src(src) {}
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
			Symbol *name;
			var_t super;
			Mirb::Block *block;
			
			ClassOp(var_t var, Symbol *name, var_t super, Mirb::Block *block) : var(var), name(name), super(super), block(block) {}
		};
		
		struct ModuleOp:
			public OpcodeWrapper<Opcode::Module>
		{
			var_t var;
			Symbol *name;
			Mirb::Block *block;
			
			ModuleOp(var_t var, Symbol *name, Mirb::Block *block) : var(var), name(name), block(block) {}
		};
		
		struct MethodOp:
			public OpcodeWrapper<Opcode::Method>
		{
			Symbol *name;
			Mirb::Block *block;
			
			MethodOp(Symbol *name, Mirb::Block *block) : name(name), block(block) {}
		};
		
		struct CallOp:
			public OpcodeWrapper<Opcode::Call>
		{
			var_t var;
			var_t obj;
			Symbol *method;
			var_t block_var;
			Mirb::Block *block;
			size_t argc;
			var_t argv;

			CallOp(var_t var, var_t obj, Symbol *method, var_t block_var, Mirb::Block *block, size_t argc, var_t argv) : var(var), obj(obj), method(method), block_var(block_var), block(block), argc(argc), argv(argv) {}
		};
		
		struct SuperOp:
			public OpcodeWrapper<Opcode::Super>
		{
			var_t var;
			var_t block_var;
			Mirb::Block *block;
			size_t argc;
			var_t argv;
			
			SuperOp(var_t var, var_t block_var, Mirb::Block *block, size_t argc, var_t argv) : var(var), block_var(block_var), block(block), argc(argc), argv(argv) {}
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
		
		struct GetConstOp:
			public OpcodeWrapper<Opcode::GetConst>
		{
			var_t var;
			var_t obj;
			Symbol *name;
			
			GetConstOp(var_t var, var_t obj, Symbol *name) : var(var), obj(obj), name(name) {}
		};
		
		struct SetConstOp:
			public OpcodeWrapper<Opcode::SetConst>
		{
			var_t obj;
			Symbol *name;
			var_t var;
			
			SetConstOp(var_t obj, Symbol *name, var_t var) : obj(obj), name(name), var(var) {}
		};
		
		struct BranchOpcode:
			public Opcode
		{
			size_t pos;

			BranchOpcode(Opcode::Type type) : Opcode(type) {}
		};
		
		struct BranchIfOp:
			public BranchOpcode
		{
			var_t var;

			BranchIfOp(var_t var) : BranchOpcode(BranchIf), var(var) {}
		};
		
		struct BranchIfZeroOp:
			public BranchOpcode
		{
			var_t var;

			BranchIfZeroOp(var_t var) : BranchOpcode(BranchIfZero), var(var) {}
		};
		
		struct BranchUnlessOp:
			public BranchOpcode
		{
			var_t var;

			BranchUnlessOp(var_t var) : BranchOpcode(BranchUnless), var(var) {}
		};
		
		struct BranchUnlessZeroOp:
			public BranchOpcode
		{
			var_t var;

			BranchUnlessZeroOp(var_t var) : BranchOpcode(BranchUnlessZero), var(var) {}
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
		
		struct UnwindOp:
			public OpcodeWrapper<Opcode::Unwind>
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
		
		struct StringOp:
			public OpcodeWrapper<Opcode::String>
		{
			var_t var;
			StringData str;

			StringOp(var_t var, const StringData &str) : var(var), str(str) {}
		};
		
		struct InterpolateOp:
			public OpcodeWrapper<Opcode::Interpolate>
		{
			var_t var;
			size_t argc;
			var_t argv;

			InterpolateOp(var_t var, size_t argc, var_t argv) : var(var), argc(argc), argv(argv) {}
		};
	};
};
