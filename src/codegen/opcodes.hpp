#pragma once
#include <functional>
#include "../../runtime/runtime.hpp"
#include "../common.hpp"
#include "../simple-list.hpp"
#include "../list.hpp"

namespace Mirb
{
	class MemoryPool;
	class Symbol;
	
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class Block;

		class BasicBlock;
		
		struct MoveOp;
		struct LoadOp;
		struct LoadRawOp;
		struct PushOp;
		struct PushImmediateOp;
		struct PushRawOp;
		struct ClosureOp;
		struct ClassOp;
		struct ModuleOp;
		struct MethodOp;
		struct CallOp;
		struct SuperOp;
		struct GetHeapOp;
		struct GetHeapVarOp;
		struct SetHeapVarOp;
		struct GetIVarOp;
		struct SetIVarOp;
		struct GetConstOp;
		struct SetConstOp;
		struct BranchIfOp;
		struct BranchUnlessOp;
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
				Push,
				PushImmediate,
				PushRaw,
				Closure,
				Class,
				Module,
				Method,
				Call,
				Super,
				GetHeap,
				GetHeapVar,
				SetHeapVar,
				GetIVar,
				SetIVar,
				GetConst,
				SetConst,
				BranchIf,
				BranchUnless,
				Branch,
				Return,
				Handler,
				Unwind,
				UnwindReturn,
				UnwindBreak,
				BreakTarget,
				Array,
				String,
				Interpolate,
			};
			
			Type op;

			Opcode(Type op) : op(op) {}
			
			SimpleEntry<Opcode> entry;

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

					case Push:
						return T<PushOp>::func(arg, (PushOp &)*this);

					case PushImmediate:
						return T<PushImmediateOp>::func(arg, (PushImmediateOp &)*this);

					case PushRaw:
						return T<PushRawOp>::func(arg, (PushRawOp &)*this);

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

					case GetHeap:
						return T<GetHeapOp>::func(arg, (GetHeapOp &)*this);
						
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

					case BranchUnless:
						return T<BranchUnlessOp>::func(arg, (BranchUnlessOp &)*this);

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

					case BreakTarget:
						return T<BreakTargetOp>::func(arg, (BreakTargetOp &)*this);
						
					case Array:
						return T<ArrayOp>::func(arg, (ArrayOp &)*this);

					case String:
						return T<StringOp>::func(arg, (StringOp &)*this);

					case Interpolate:
						return T<InterpolateOp>::func(arg, (InterpolateOp &)*this);

					default:
						debug_fail("Unknown opcode type");
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
			Tree::Variable *dst;
			Tree::Variable *src;

			template<typename T> void def(T def) { def(dst); };
			template<typename T> void use(T use) { use(src); };
			
			MoveOp(Tree::Variable *dst, Tree::Variable *src) : dst(dst), src(src) {}
		};
		
		struct LoadOp:
			public OpcodeWrapper<Opcode::Load>
		{
			Tree::Variable *var;
			rt_value imm;
			
			template<typename T> void def(T def) { def(var); };

			LoadOp(Tree::Variable *var, rt_value imm) : var(var), imm(imm) {}
		};
		
		struct LoadRawOp:
			public OpcodeWrapper<Opcode::LoadRaw>
		{
			Tree::Variable *var;
			size_t imm;
			
			template<typename T> void def(T def) { def(var); };

			LoadRawOp(Tree::Variable *var, size_t imm) : var(var), imm(imm) {}
		};
		
		struct PushOp:
			public OpcodeWrapper<Opcode::Push>
		{
			Tree::Variable *var;

			template<typename T> void use(T use) { use(var); };

			PushOp(Tree::Variable *var) : var(var) {}
		};
		
		struct PushImmediateOp:
			public OpcodeWrapper<Opcode::PushImmediate>
		{
			rt_value imm;
			
			PushImmediateOp(rt_value imm) : imm(imm) {}
		};
		
		struct PushRawOp:
			public OpcodeWrapper<Opcode::PushRaw>
		{
			size_t imm;
			
			PushRawOp(size_t imm) : imm(imm) {}
		};
		
		struct ClosureOp:
			public OpcodeWrapper<Opcode::Closure>
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Block *block;
			size_t scope_count;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(self); };

			ClosureOp(Tree::Variable *var, Tree::Variable *self, Block *block, size_t scope_count) : var(var), self(self), block(block), scope_count(scope_count) {}
		};
		
		struct ClassOp:
			public OpcodeWrapper<Opcode::Class>
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			Tree::Variable *super;
			Block *block;
			
			template<typename T> void def(T def) { if(var) def(var); };

			template<typename T> void use(T use)
			{
				use(self);
				use(super);
			};

			ClassOp(Tree::Variable *var, Tree::Variable *self, Symbol *name, Tree::Variable *super, Block *block) : var(var), self(self), name(name), super(super), block(block) {}
		};
		
		struct ModuleOp:
			public OpcodeWrapper<Opcode::Module>
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			Block *block;
			
			template<typename T> void def(T def) { if(var) def(var); };
			template<typename T> void use(T use) { use(self); };

			ModuleOp(Tree::Variable *var, Tree::Variable *self, Symbol *name, Block *block) : var(var), self(self), name(name), block(block) {}
		};
		
		struct MethodOp:
			public OpcodeWrapper<Opcode::Method>
		{
			Tree::Variable *self;
			Symbol *name;
			Block *block;
			
			template<typename T> void use(T use) { use(self); };
			
			MethodOp(Tree::Variable *self, Symbol *name, Block *block) : self(self), name(name), block(block) {}
		};
		
		struct CallOp:
			public OpcodeWrapper<Opcode::Call>
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			Symbol *method;
			size_t param_count;
			Tree::Variable *block;
			size_t break_id;
			
			template<typename T> void def(T def) { if(var) def(var); };
			
			template<typename T> void use(T use)
			{
				use(obj);

				if(block)
					use(block);
			};

			CallOp(Tree::Variable *var, Tree::Variable *obj, Symbol *method, size_t param_count, Tree::Variable *block, size_t break_id) : var(var), obj(obj), method(method), param_count(param_count), block(block), break_id(break_id) {}
		};
		
		struct SuperOp:
			public OpcodeWrapper<Opcode::Super>
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Tree::Variable *module;
			Tree::Variable *method;
			size_t param_count;
			Tree::Variable *block;
			size_t break_id;
			
			template<typename T> void def(T def) { if(var) def(var); };
			
			template<typename T> void use(T use)
			{
				use(self);
				use(module);
				use(method);

				if(block)
					use(block);
			};

			SuperOp(Tree::Variable *var, Tree::Variable *self, Tree::Variable *module, Tree::Variable *method, size_t param_count, Tree::Variable *block, size_t break_id) : var(var), self(self), module(module), method(method), param_count(param_count), block(block), break_id(break_id) {}
		};
		
		struct GetHeapOp:
			public OpcodeWrapper<Opcode::GetHeap>
		{
			Tree::Variable *var;
			Tree::Variable *heaps;
			size_t index;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(heaps); };

			GetHeapOp(Tree::Variable *var, Tree::Variable *heaps, size_t index) : var(var), heaps(heaps), index(index) {}
		};
		
		struct GetHeapVarOp:
			public OpcodeWrapper<Opcode::GetHeapVar>
		{
			Tree::Variable *var;
			Tree::Variable *heap;
			Tree::Variable *index;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(heap); };

			GetHeapVarOp(Tree::Variable *var, Tree::Variable *heap, Tree::Variable *index) : var(var), heap(heap), index(index) {}
		};
		
		struct SetHeapVarOp:
			public OpcodeWrapper<Opcode::SetHeapVar>
		{
			Tree::Variable *heap;
			Tree::Variable *index;
			Tree::Variable *var;
			
			template<typename T> void use(T use)
			{
				use(heap);
				use(var);
			};
			
			SetHeapVarOp(Tree::Variable *heap, Tree::Variable *index, Tree::Variable *var) : heap(heap), index(index), var(var) {}
		};
		
		struct GetIVarOp:
			public OpcodeWrapper<Opcode::GetIVar>
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(self); };

			GetIVarOp(Tree::Variable *var, Tree::Variable *self, Symbol *name) : var(var), self(self), name(name) {}
		};
		
		struct SetIVarOp:
			public OpcodeWrapper<Opcode::SetIVar>
		{
			Tree::Variable *self;
			Symbol *name;
			Tree::Variable *var;
			
			template<typename T> void use(T use)
			{
				use(self);
				use(var);
			};
			
			SetIVarOp(Tree::Variable *self, Symbol *name, Tree::Variable *var) : self(self), name(name), var(var) {}
		};
		
		struct GetConstOp:
			public OpcodeWrapper<Opcode::GetConst>
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			Symbol *name;
			
			template<typename T> void def(T def) { def(var); };
			template<typename T> void use(T use) { use(obj); };

			GetConstOp(Tree::Variable *var, Tree::Variable *obj, Symbol *name) : var(var), obj(obj), name(name) {}
		};
		
		struct SetConstOp:
			public OpcodeWrapper<Opcode::SetConst>
		{
			Tree::Variable *obj;
			Symbol *name;
			Tree::Variable *var;
			
			template<typename T> void use(T use)
			{
				use(obj);
				use(var);
			};
			
			SetConstOp(Tree::Variable *obj, Symbol *name, Tree::Variable *var) : obj(obj), name(name), var(var) {}
		};
		
		struct BranchIfOp:
			public OpcodeWrapper<Opcode::BranchIf>
		{
			BasicBlock *ltrue;
			Tree::Variable *var;
			
			template<typename T> void use(T use) { use(var); };
			
			BranchIfOp(BasicBlock *ltrue, Tree::Variable *var) : ltrue(ltrue), var(var) {}
		};
		
		struct BranchUnlessOp:
			public OpcodeWrapper<Opcode::BranchUnless>
		{
			BasicBlock *lfalse;
			Tree::Variable *var;
			
			template<typename T> void use(T use) { use(var); };
			
			BranchUnlessOp(BasicBlock *lfalse, Tree::Variable *var) : lfalse(lfalse), var(var) {}
		};
		
		struct BranchOp:
			public OpcodeWrapper<Opcode::Branch>
		{
			BasicBlock *label;
			
			BranchOp(BasicBlock *label) : label(label) {}
		};
		
		struct ReturnOp:
			public OpcodeWrapper<Opcode::Return>
		{
			Tree::Variable *var;
			
			template<typename T> void use(T use) { use(var); };
			
			ReturnOp(Tree::Variable *var) : var(var) {}
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
			Tree::Variable *var;
			void *code;
			
			template<typename T> void use(T use) { use(var); };
			
			UnwindReturnOp(Tree::Variable *var, void *code) : var(var), code(code) {}
		};
		
		struct UnwindBreakOp:
			public OpcodeWrapper<Opcode::UnwindBreak>
		{
			Tree::Variable *var;
			void *code;
			size_t index;
			
			template<typename T> void use(T use) { use(var); };
			
			UnwindBreakOp(Tree::Variable *var, void *code, size_t index) : var(var), code(code), index(index) {}
		};
		
		struct BreakTargetOp:
			public OpcodeWrapper<Opcode::BreakTarget>
		{
		};
		
		struct ArrayOp:
			public OpcodeWrapper<Opcode::Array>
		{
			Tree::Variable *var;
			size_t element_count;
			
			template<typename T> void def(T def) { def(var); };

			ArrayOp(Tree::Variable *var, size_t element_count) : var(var), element_count(element_count) {}
		};
		
		struct StringOp:
			public OpcodeWrapper<Opcode::String>
		{
			Tree::Variable *var;
			const char_t *str;
			
			template<typename T> void def(T def) { def(var); };

			StringOp(Tree::Variable *var, const char_t *str) : var(var), str(str) {}
		};
		
		struct InterpolateOp:
			public OpcodeWrapper<Opcode::Interpolate>
		{
			Tree::Variable *var;
			size_t param_count;
			
			template<typename T> void def(T def) { def(var); };

			InterpolateOp(Tree::Variable *var, size_t param_count) : var(var), param_count(param_count) {}
		};
	};
};
