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
		struct LoadRawOp;
		struct LoadArgOp;
		struct GroupOp;
		struct ClosureOp;
		struct ClassOp;
		struct ModuleOp;
		struct MethodOp;
		struct CallOp;
		struct SuperOp;
		struct LookupOp;
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
		struct StaticCallOp;
		struct SetupVarsOp;
		struct ReturnOp;
		struct RaiseOp;
		
		struct Opcode
		{
			enum Type
			{
				Move,
				Load,
				LoadRaw,
				LoadArg,
				Group,
				Closure,
				Class,
				Module,
				Method,
				Call,
				Super,
				Lookup,
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
				Interpolate,
				StaticCall,
				Raise,
				SetupVars
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
						
					case LoadRaw:
						return T<LoadRawOp>::func(arg, (LoadRawOp &)*this);
						
					case LoadArg:
						return T<LoadArgOp>::func(arg, (LoadArgOp &)*this);

					case Group:
						return T<GroupOp>::func(arg, (GroupOp &)*this);

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
						
					case Flush:
						return T<FlushOp>::func(arg, (FlushOp &)*this);
						
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
					
					case StaticCall:
						return T<StaticCallOp>::func(arg, (StaticCallOp &)*this);
						
					case Raise:
						return T<RaiseOp>::func(arg, (RaiseOp &)*this);

					case SetupVars:
						return T<SetupVarsOp>::func(arg, (SetupVarsOp &)*this);

					default:
						mirb_debug_abort("Unknown opcode type");
				};
			}

			template<typename T> void def(T def) { };
			template<typename T> void use(T use) { };
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

			template<typename T> void def(T def) { def(dst); };
			template<typename T> void use(T use) { use(src); };
			
			MoveOp(var_t dst, var_t src) : dst(dst), src(src) {}
		};
		
		struct LoadOp:
			public OpcodeWrapper<Opcode::Load>
		{
			var_t var;
			value_t imm;
			
			template<typename T> void def(T def) { def(var); };

			LoadOp(var_t var, value_t imm) : var(var), imm(imm) {}
		};
		
		struct LoadRawOp:
			public OpcodeWrapper<Opcode::LoadRaw>
		{
			var_t var;
			size_t imm;
			
			template<typename T> void def(T def) { def(var); };

			LoadRawOp(var_t var, size_t imm) : var(var), imm(imm) {}
		};
		
		struct LoadArgOp:
			public OpcodeWrapper<Opcode::LoadArg>
		{
			var_t var;
			size_t arg;
			
			template<typename T> void def(T def) { def(var); };

			LoadArgOp(var_t var, size_t arg) : var(var), arg(arg) {}
		};
		
		struct GroupOp:
			public OpcodeWrapper<Opcode::Group>
		{
			var_t var;
			size_t address;
			
			template<typename T> void def(T def) { def(var); };

			GroupOp(var_t var, size_t address) : var(var), address(address) {}
		};

		struct ClosureOp:
			public OpcodeWrapper<Opcode::Closure>
		{
			var_t var;
			var_t self;
			var_t name;
			var_t module;
			Mirb::Block *block;
			size_t argc;
			var_t argv;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use)
			{
				use(self);
				use(name);
				use(module);
				use(argv);
			};

			ClosureOp(var_t var, var_t self, var_t name, var_t module, Mirb::Block *block, size_t argc, var_t argv) : var(var), self(self), name(name), module(module), block(block), argc(argc), argv(argv) {}
		};
		
		struct ClassOp:
			public OpcodeWrapper<Opcode::Class>
		{
			var_t var;
			var_t self;
			Symbol *name;
			var_t super;
			Mirb::Block *block;
			
			template<typename T> void def(T def) { if(var) def(var); };

			template<typename T> void use(T use)
			{
				use(self);
				
				if(super)
					use(super);
			};

			ClassOp(var_t var, var_t self, Symbol *name, var_t super, Mirb::Block *block) : var(var), self(self), name(name), super(super), block(block) {}
		};
		
		struct ModuleOp:
			public OpcodeWrapper<Opcode::Module>
		{
			var_t var;
			var_t self;
			Symbol *name;
			Mirb::Block *block;
			
			template<typename T> void def(T def) { if(var) def(var); };
			template<typename T> void use(T use) { use(self); };

			ModuleOp(var_t var, var_t self, Symbol *name, Mirb::Block *block) : var(var), self(self), name(name), block(block) {}
		};
		
		struct MethodOp:
			public OpcodeWrapper<Opcode::Method>
		{
			var_t self;
			Symbol *name;
			Mirb::Block *block;
			
			template<typename T> void use(T use) { use(self); };
			
			MethodOp(var_t self, Symbol *name, Mirb::Block *block) : self(self), name(name), block(block) {}
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
			
			template<typename T> void def(T def) { if(var) def(var); };
			
			template<typename T> void use(T use)
			{
				use(obj);
				
				if(argv)
					use(argv);
				
				if(block_var)
					use(block_var);
			};

			CallOp(var_t var, var_t obj, Symbol *method, var_t block_var, Mirb::Block *block, size_t argc, var_t argv) : var(var), obj(obj), method(method), block_var(block_var), block(block), argc(argc), argv(argv) {}
		};
		
		struct SuperOp:
			public OpcodeWrapper<Opcode::Super>
		{
			var_t var;
			var_t self;
			var_t module;
			var_t method;
			var_t block_var;
			Mirb::Block *block;
			size_t argc;
			var_t argv;
			
			template<typename T> void def(T def) { if(var) def(var); };
			
			template<typename T> void use(T use)
			{
				use(self);
				use(module);
				use(method);

				if(argv)
					use(argv);

				if(block_var)
					use(block_var);
			};

			SuperOp(var_t var, var_t self, var_t module, var_t method, var_t block_var, Mirb::Block *block, size_t argc, var_t argv) : var(var), self(self), module(module), method(method), block_var(block_var), block(block), argc(argc), argv(argv) {}
		};
		
		struct LookupOp:
			public OpcodeWrapper<Opcode::Lookup>
		{
			var_t var;
			var_t array_var;
			size_t index;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(array_var); };

			LookupOp(var_t var, var_t array_var, size_t index) : var(var), array_var(array_var), index(index) {}
		};
		
		struct CreateHeapOp:
			public OpcodeWrapper<Opcode::CreateHeap>
		{
			var_t var;
			
			template<typename T> void def(T def) { def(var); };

			CreateHeapOp(var_t var) : var(var) {}
		};
		
		struct GetHeapVarOp:
			public OpcodeWrapper<Opcode::GetHeapVar>
		{
			var_t var;
			var_t heap;
			var_t index;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(heap); };

			GetHeapVarOp(var_t var, var_t heap, var_t index) : var(var), heap(heap), index(index) {}
		};
		
		struct SetHeapVarOp:
			public OpcodeWrapper<Opcode::SetHeapVar>
		{
			var_t heap;
			var_t index;
			var_t var;
			
			template<typename T> void use(T use)
			{
				use(heap);
				use(var);
			};
			
			SetHeapVarOp(var_t heap, var_t index, var_t var) : heap(heap), index(index), var(var) {}
		};
		
		struct GetIVarOp:
			public OpcodeWrapper<Opcode::GetIVar>
		{
			var_t var;
			var_t self;
			Symbol *name;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(self); };

			GetIVarOp(var_t var, var_t self, Symbol *name) : var(var), self(self), name(name) {}
		};
		
		struct SetIVarOp:
			public OpcodeWrapper<Opcode::SetIVar>
		{
			var_t self;
			Symbol *name;
			var_t var;
			
			template<typename T> void use(T use)
			{
				use(self);
				use(var);
			};
			
			SetIVarOp(var_t self, Symbol *name, var_t var) : self(self), name(name), var(var) {}
		};
		
		struct GetConstOp:
			public OpcodeWrapper<Opcode::GetConst>
		{
			var_t var;
			var_t obj;
			Symbol *name;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(obj); };

			GetConstOp(var_t var, var_t obj, Symbol *name) : var(var), obj(obj), name(name) {}
		};
		
		struct SetConstOp:
			public OpcodeWrapper<Opcode::SetConst>
		{
			var_t obj;
			Symbol *name;
			var_t var;
			
			template<typename T> void use(T use)
			{
				use(obj);
				use(var);
			};
			
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
			
			template<typename T> void use(T use) { use(var); };
			
			BranchIfOp(var_t var) : BranchOpcode(BranchIf), var(var) {}
		};
		
		struct BranchIfZeroOp:
			public BranchOpcode
		{
			var_t var;
			
			template<typename T> void use(T use) { use(var); };
			
			BranchIfZeroOp(var_t var) : BranchOpcode(BranchIfZero), var(var) {}
		};
		
		struct BranchUnlessOp:
			public BranchOpcode
		{
			var_t var;
			
			template<typename T> void use(T use) { use(var); };
			
			BranchUnlessOp(var_t var) : BranchOpcode(BranchUnless), var(var) {}
		};
		
		struct BranchUnlessZeroOp:
			public BranchOpcode
		{
			var_t var;
			
			template<typename T> void use(T use) { use(var); };
			
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
			ReturnOp() {}
		};
		
		struct HandlerOp:
			public OpcodeWrapper<Opcode::Handler>
		{
			size_t id;
			
			HandlerOp(size_t id) : id(id) {}
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
			
			template<typename T> void use(T use) { use(var); };
			
			UnwindReturnOp(var_t var, Mirb::Block *code) : var(var), code(code) {}
		};
		
		struct UnwindBreakOp:
			public OpcodeWrapper<Opcode::UnwindBreak>
		{
			var_t var;
			Mirb::Block *code;
			size_t index;
			
			template<typename T> void use(T use) { use(var); };
			
			UnwindBreakOp(var_t var, Mirb::Block *code, size_t index) : var(var), code(code), index(index) {}
		};
		
		struct ArrayOp:
			public OpcodeWrapper<Opcode::Array>
		{
			var_t var;
			size_t argc;
			var_t argv;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(argv); };

			ArrayOp(var_t var, size_t argc, var_t argv) : var(var), argc(argc), argv(argv) {}
		};
		
		struct StringOp:
			public OpcodeWrapper<Opcode::String>
		{
			var_t var;
			const char_t *str;
			
			template<typename T> void def(T def) { def(var); };

			StringOp(var_t var, const char_t *str) : var(var), str(str) {}
		};
		
		struct InterpolateOp:
			public OpcodeWrapper<Opcode::Interpolate>
		{
			var_t var;
			size_t argc;
			var_t argv;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(argv); };

			InterpolateOp(var_t var, size_t argc, var_t argv) : var(var), argc(argc), argv(argv) {}
		};
		
		struct StaticCallOp:
			public OpcodeWrapper<Opcode::StaticCall>
		{
			var_t var;
			void *function;
			var_t *args;
			size_t arg_count;
			
			template<typename T> void def(T def) { def(var); };

			template<typename T> void use(T use)
			{
				for(size_t i = 0; i < arg_count; ++i)
					use(args[i]);
			};

			StaticCallOp(var_t var, void *function, var_t *args, size_t arg_count) : var(var), function(function), args(args), arg_count(arg_count) {}
		};
		
		struct RaiseOp:
			public OpcodeWrapper<Opcode::Raise>
		{
		};
		
		struct SetupVarsOp:
			public OpcodeWrapper<Opcode::SetupVars>
		{
			var_t *args;
			size_t arg_count;

			SetupVarsOp(var_t *args, size_t arg_count) : args(args), arg_count(arg_count) {}
		};
	};
};
