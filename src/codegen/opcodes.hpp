#pragma once
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

		struct Opcode
		{
			enum Type
			{
				None,
				Move,
				Load,
				LoadRaw,
				Push,
				PushImmediate,
				PushRaw,
				PushScope,
				Closure,
				Class,
				Module,
				Method,
				Call,
				Super,
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
			
			virtual Type op() { return None; }
			
			SimpleEntry<Opcode> entry;

			virtual void def_use(BasicBlock &block) {}
		};
		
		struct MoveOp:
			public Opcode
		{
			Type op() { return Move; }

			Tree::Variable *dst;
			Tree::Variable *src;

			void def_use(BasicBlock &block);
			
			MoveOp(Tree::Variable *dst, Tree::Variable *src) : dst(dst), src(src) {}
		};
		
		struct LoadOp:
			public Opcode
		{
			Type op() { return Load; }

			Tree::Variable *var;
			rt_value imm;
			
			void def_use(BasicBlock &block);
			
			LoadOp(Tree::Variable *var, rt_value imm) : var(var), imm(imm) {}
		};
		
		struct LoadRawOp:
			public Opcode
		{
			Type op() { return LoadRaw; }

			Tree::Variable *var;
			size_t imm;
			
			void def_use(BasicBlock &block);
			
			LoadRawOp(Tree::Variable *var, size_t imm) : var(var), imm(imm) {}
		};
		
		struct PushOp:
			public Opcode
		{
			Type op() { return Push; }

			Tree::Variable *var;
			
			void def_use(BasicBlock &block);
			
			PushOp(Tree::Variable *var) : var(var) {}
		};
		
		struct PushImmediateOp:
			public Opcode
		{
			Type op() { return PushImmediate; }

			rt_value imm;
			
			PushImmediateOp(rt_value imm) : imm(imm) {}
		};
		
		struct PushRawOp:
			public Opcode
		{
			Type op() { return PushRaw; }

			size_t imm;
			
			PushRawOp(size_t imm) : imm(imm) {}
		};
		
		struct PushScopeOp:
			public Opcode
		{
			Type op() { return PushScope; }

			Block *block;
			
			PushScopeOp(Block *block) : block(block) {}
		};
		
		struct ClosureOp:
			public Opcode
		{
			Type op() { return Closure; }

			Tree::Variable *var;
			Tree::Variable *self;
			Block *block;
			size_t scope_count;
			
			void def_use(BasicBlock &block);
			
			ClosureOp(Tree::Variable *var, Tree::Variable *self, Block *block, size_t scope_count) : var(var), self(self), block(block), scope_count(scope_count) {}
		};
		
		struct ClassOp:
			public Opcode
		{
			Type op() { return Class; }

			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			Tree::Variable *super;
			Block *block;
			
			void def_use(BasicBlock &block);
			
			ClassOp(Tree::Variable *var, Tree::Variable *self, Symbol *name, Tree::Variable *super, Block *block) : var(var), self(self), name(name), super(super), block(block) {}
		};
		
		struct ModuleOp:
			public Opcode
		{
			Type op() { return Module; }

			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			Block *block;
			
			void def_use(BasicBlock &block);
			
			ModuleOp(Tree::Variable *var, Tree::Variable *self, Symbol *name, Block *block) : var(var), self(self), name(name), block(block) {}
		};
		
		struct MethodOp:
			public Opcode
		{
			Type op() { return Method; }

			Tree::Variable *self;
			Symbol *name;
			Block *block;
			
			void def_use(BasicBlock &block);
			
			MethodOp(Tree::Variable *self, Symbol *name, Block *block) : self(self), name(name), block(block) {}
		};
		
		struct CallOp:
			public Opcode
		{
			Type op() { return Call; }

			Tree::Variable *var;
			Tree::Variable *obj;
			Symbol *method;
			size_t param_count;
			Tree::Variable *block;
			size_t break_id;
			
			void def_use(BasicBlock &block);
			
			CallOp(Tree::Variable *var, Tree::Variable *obj, Symbol *method, size_t param_count, Tree::Variable *block, size_t break_id) : var(var), obj(obj), method(method), param_count(param_count), block(block), break_id(break_id) {}
		};
		
		struct SuperOp:
			public Opcode
		{
			Type op() { return Super; }

			Tree::Variable *var;
			Tree::Variable *self;
			Tree::Variable *module;
			Tree::Variable *method;
			size_t param_count;
			Tree::Variable *block;
			size_t break_id;
			
			void def_use(BasicBlock &block);
			
			SuperOp(Tree::Variable *var, Tree::Variable *self, Tree::Variable *module, Tree::Variable *method, size_t param_count, Tree::Variable *block, size_t break_id) : var(var), self(self), module(module), method(method), param_count(param_count), block(block), break_id(break_id) {}
		};
		
		struct GetIVarOp:
			public Opcode
		{
			Type op() { return GetIVar; }

			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			
			void def_use(BasicBlock &block);
			
			GetIVarOp(Tree::Variable *var, Tree::Variable *self, Symbol *name) : var(var), self(self), name(name) {}
		};
		
		struct SetIVarOp:
			public Opcode
		{
			Type op() { return SetIVar; }

			Tree::Variable *self;
			Symbol *name;
			Tree::Variable *var;
			
			void def_use(BasicBlock &block);
			
			SetIVarOp(Tree::Variable *self, Symbol *name, Tree::Variable *var) : self(self), name(name), var(var) {}
		};
		
		struct GetConstOp:
			public Opcode
		{
			Type op() { return GetConst; }

			Tree::Variable *var;
			Tree::Variable *obj;
			Symbol *name;
			
			void def_use(BasicBlock &block);
			
			GetConstOp(Tree::Variable *var, Tree::Variable *obj, Symbol *name) : var(var), obj(obj), name(name) {}
		};
		
		struct SetConstOp:
			public Opcode
		{
			Type op() { return SetConst; }

			Tree::Variable *obj;
			Symbol *name;
			Tree::Variable *var;
			
			void def_use(BasicBlock &block);
			
			SetConstOp(Tree::Variable *obj, Symbol *name, Tree::Variable *var) : obj(obj), name(name), var(var) {}
		};
		
		struct BranchIfOp:
			public Opcode
		{
			Type op() { return BranchIf; }

			BasicBlock *ltrue;
			Tree::Variable *var;
			
			void def_use(BasicBlock &block);
			
			BranchIfOp(BasicBlock *ltrue, Tree::Variable *var) : ltrue(ltrue), var(var) {}
		};
		
		struct BranchUnlessOp:
			public Opcode
		{
			Type op() { return BranchUnless; }

			BasicBlock *lfalse;
			Tree::Variable *var;
			
			void def_use(BasicBlock &block);
			
			BranchUnlessOp(BasicBlock *lfalse, Tree::Variable *var) : lfalse(lfalse), var(var) {}
		};
		
		struct BranchOp:
			public Opcode
		{
			Type op() { return Branch; }

			BasicBlock *label;
			
			BranchOp(BasicBlock *label) : label(label) {}
		};
		
		struct ReturnOp:
			public Opcode
		{
			Type op() { return Return; }

			Tree::Variable *var;
			
			void def_use(BasicBlock &block);
			
			ReturnOp(Tree::Variable *var) : var(var) {}
		};
		
		struct HandlerOp:
			public Opcode
		{
			Type op() { return Handler; }

			size_t id;
			
			HandlerOp(size_t id) : id(id) {}
		};
		
		struct UnwindOp:
			public Opcode
		{
			Type op() { return Unwind; }
		};
		
		struct UnwindReturnOp:
			public Opcode
		{
			Type op() { return UnwindReturn; }

			Tree::Variable *var;
			void *code;
			
			void def_use(BasicBlock &block);
			
			UnwindReturnOp(Tree::Variable *var, void *code) : var(var), code(code) {}
		};
		
		struct UnwindBreakOp:
			public Opcode
		{
			Type op() { return UnwindBreak; }

			Tree::Variable *var;
			void *code;
			size_t index;
			
			void def_use(BasicBlock &block);
			
			UnwindBreakOp(Tree::Variable *var, void *code, size_t index) : var(var), code(code), index(index) {}
		};
		
		struct BreakTargetOp:
			public Opcode
		{
			Type op() { return BreakTarget; }
		};
		
		struct ArrayOp:
			public Opcode
		{
			Type op() { return Array; }

			Tree::Variable *var;
			size_t element_count;
			
			void def_use(BasicBlock &block);
			
			ArrayOp(Tree::Variable *var, size_t element_count) : var(var), element_count(element_count) {}
		};
		
		struct StringOp:
			public Opcode
		{
			Type op() { return String; }

			Tree::Variable *var;
			const char_t *str;
			
			void def_use(BasicBlock &block);
			
			StringOp(Tree::Variable *var, const char_t *str) : var(var), str(str) {}
		};
		
		struct InterpolateOp:
			public Opcode
		{
			Type op() { return Interpolate; }

			Tree::Variable *var;
			size_t param_count;
			
			void def_use(BasicBlock &block);
			
			InterpolateOp(Tree::Variable *var, size_t param_count) : var(var), param_count(param_count) {}
		};
	};
};
