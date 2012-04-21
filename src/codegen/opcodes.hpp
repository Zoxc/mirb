#pragma once
#include "../common.hpp"
#include "../value.hpp"
#include "../block.hpp"
#include "../generic/simple-list.hpp"
#include "../generic/simpler-list.hpp"
#include "../generic/list.hpp"

namespace Mirb
{
	class MemoryPool;
	class Symbol;
	class Block;
	
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class BasicBlock;
		
		struct MoveOp;
		struct LoadOp;
		struct LoadArgOp;
		struct ClosureOp;
		struct ClassOp;
		struct ModuleOp;
		struct MethodOp;
		struct CallOp;
		struct SuperOp;
		struct LookupOp;
		struct SelfOp;
		struct BlockOp;
		struct CreateHeapOp;
		struct GetHeapVarOp;
		struct SetHeapVarOp;
		struct GetIVarOp;
		struct SetIVarOp;
		struct GetConstOp;
		struct SetConstOp;
		struct BranchIfOp;
		struct BranchIfZeroOp;
		struct BranchUnlessOp;
		struct BranchUnlessZeroOp;
		struct BranchOp;
		struct ReturnOp;
		struct HandlerOp;
		struct UnwindOp;
		struct UnwindReturnOp;
		struct UnwindBreakOp;
		struct BreakTargetOp;
		struct ArrayOp;
		struct StringOp;
		struct InterpolateOp;
		
		struct Opcode
		{
			enum Type
			{
				Move,
				Load,
				LoadRaw,
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
				Array,
				String,
				Interpolate
			};
			
			Type op;

			Opcode(Type op) : op(op) {}
			
			template<template<typename> class T, typename Arg> auto virtual_do(Arg arg) -> decltype(T<Opcode>::func(arg, *(Opcode *)0))
			{
				switch(op)
				{
					case Move:
						return T<MoveOp>::func(arg, (MoveOp &)*this);
						
					case Load:
						return T<LoadOp>::func(arg, (LoadOp &)*this);
						
					case LoadArg:
						return T<LoadArgOp>::func(arg, (LoadArgOp &)*this);

					case Closure:
						return T<ClosureOp>::func(arg, (ClosureOp &)*this);

					case Class:
						return T<ClassOp>::func(arg, (ClassOp &)*this);

					case Module:
						return T<ModuleOp>::func(arg, (ModuleOp &)*this);

					case Method:
						return T<MethodOp>::func(arg, (MethodOp &)*this);

					case Call:
						return T<CallOp>::func(arg, (CallOp &)*this);
						
					case Super:
						return T<SuperOp>::func(arg, (SuperOp &)*this);
						
					case Lookup:
						return T<LookupOp>::func(arg, (LookupOp &)*this);
						
					case CreateHeap:
						return T<CreateHeapOp>::func(arg, (CreateHeapOp &)*this);
						
					case GetHeapVar:
						return T<GetHeapVarOp>::func(arg, (GetHeapVarOp &)*this);

					case SetHeapVar:
						return T<SetHeapVarOp>::func(arg, (SetHeapVarOp &)*this);

					case GetIVar:
						return T<GetIVarOp>::func(arg, (GetIVarOp &)*this);

					case SetIVar:
						return T<SetIVarOp>::func(arg, (SetIVarOp &)*this);

					case GetConst:
						return T<GetConstOp>::func(arg, (GetConstOp &)*this);

					case SetConst:
						return T<SetConstOp>::func(arg, (SetConstOp &)*this);
						
					case BranchIf:
						return T<BranchIfOp>::func(arg, (BranchIfOp &)*this);
						
					case BranchIfZero:
						return T<BranchIfZeroOp>::func(arg, (BranchIfZeroOp &)*this);
						
					case BranchUnless:
						return T<BranchUnlessOp>::func(arg, (BranchUnlessOp &)*this);

					case BranchUnlessZero:
						return T<BranchUnlessZeroOp>::func(arg, (BranchUnlessZeroOp &)*this);

					case Branch:
						return T<BranchOp>::func(arg, (BranchOp &)*this);

					case Return:
						return T<ReturnOp>::func(arg, (ReturnOp &)*this);

					case Handler:
						return T<HandlerOp>::func(arg, (HandlerOp &)*this);

					case Unwind:
						return T<UnwindOp>::func(arg, (UnwindOp &)*this);

					case UnwindReturn:
						return T<UnwindReturnOp>::func(arg, (UnwindReturnOp &)*this);

					case UnwindBreak:
						return T<UnwindBreakOp>::func(arg, (UnwindBreakOp &)*this);

					case Array:
						return T<ArrayOp>::func(arg, (ArrayOp &)*this);

					case String:
						return T<StringOp>::func(arg, (StringOp &)*this);
					
					case Interpolate:
						return T<InterpolateOp>::func(arg, (InterpolateOp &)*this);
					
					default:
						mirb_debug_abort("Unknown opcode type");
				};
			}
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
		
		struct LoadOp:
			public OpcodeWrapper<Opcode::Load>
		{
			var_t var;
			value_t imm;

			LoadOp(var_t var, value_t imm) : var(var), imm(imm) {}
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
			const char_t *str;

			StringOp(var_t var, const char_t *str) : var(var), str(str) {}
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
