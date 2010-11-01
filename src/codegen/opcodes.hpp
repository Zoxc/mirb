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
				Branch,
				BranchIf,
				BranchUnless,
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
			
			SimpleEntry<Opcode> entry;
			
			Opcode(Type op) : op(op) {}
		};
		
		struct MoveOp:
			public Opcode
		{
			Tree::Variable *dst;
			Tree::Variable *src;
			
			MoveOp(Tree::Variable *dst, Tree::Variable *src) : Opcode(Move), dst(dst), src(src) {}
		};
		
		struct LoadOp:
			public Opcode
		{
			Tree::Variable *var;
			rt_value imm;
			
			LoadOp(Tree::Variable *var, rt_value imm) : Opcode(Load), var(var), imm(imm) {}
		};
		
		struct LoadRawOp:
			public Opcode
		{
			Tree::Variable *var;
			size_t imm;
			
			LoadRawOp(Tree::Variable *var, size_t imm) : Opcode(LoadRaw), var(var), imm(imm) {}
		};
		
		struct PushOp:
			public Opcode
		{
			Tree::Variable *var;
			
			PushOp(Tree::Variable *var) : Opcode(Push), var(var) {}
		};
		
		struct PushImmediateOp:
			public Opcode
		{
			rt_value imm;
			
			PushImmediateOp(rt_value imm) : Opcode(PushImmediate), imm(imm) {}
		};
		
		struct PushRawOp:
			public Opcode
		{
			size_t imm;
			
			PushRawOp(size_t imm) : Opcode(PushRaw), imm(imm) {}
		};
		
		struct PushScopeOp:
			public Opcode
		{
			Block *block;
			
			PushScopeOp(Block *block) : Opcode(PushScope), block(block) {}
		};
		
		struct ClosureOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Block *block;
			size_t scope_count;
			
			ClosureOp(Tree::Variable *var, Tree::Variable *self, Block *block, size_t scope_count) : Opcode(Closure), var(var), self(self), block(block), scope_count(scope_count) {}
		};
		
		struct ClassOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			Tree::Variable *super;
			Block *block;
			
			ClassOp(Tree::Variable *var, Tree::Variable *self, Symbol *name, Tree::Variable *super, Block *block) : Opcode(Class), var(var), self(self), name(name), super(super), block(block) {}
		};
		
		struct ModuleOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			Block *block;
			
			ModuleOp(Tree::Variable *var, Tree::Variable *self, Symbol *name, Block *block) : Opcode(Module), var(var), self(self), name(name), block(block) {}
		};
		
		struct MethodOp:
			public Opcode
		{
			Tree::Variable *self;
			Symbol *name;
			Block *block;
			
			MethodOp(Tree::Variable *self, Symbol *name, Block *block) : Opcode(Method), self(self), name(name), block(block) {}
		};
		
		struct CallOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			Symbol *method;
			size_t param_count;
			Tree::Variable *block;
			size_t break_id;
			
			CallOp(Tree::Variable *var, Tree::Variable *obj, Symbol *method, size_t param_count, Tree::Variable *block, size_t break_id) : Opcode(Call), var(var), obj(obj), method(method), param_count(param_count), block(block), break_id(break_id) {}
		};
		
		struct SuperOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Tree::Variable *module;
			Tree::Variable *method;
			size_t param_count;
			Tree::Variable *block;
			size_t break_id;
			
			SuperOp(Tree::Variable *var, Tree::Variable *self, Tree::Variable *module, Tree::Variable *method, size_t param_count, Tree::Variable *block, size_t break_id) : Opcode(Super), var(var), self(self), module(module), method(method), param_count(param_count), block(block), break_id(break_id) {}
		};
		
		struct GetIVarOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			Symbol *name;
			
			GetIVarOp(Tree::Variable *var, Tree::Variable *self, Symbol *name) : Opcode(GetIVar), var(var), self(self), name(name) {}
		};
		
		struct SetIVarOp:
			public Opcode
		{
			Tree::Variable *self;
			Symbol *name;
			Tree::Variable *var;
			
			SetIVarOp(Tree::Variable *self, Symbol *name, Tree::Variable *var) : Opcode(SetIVar), self(self), name(name), var(var) {}
		};
		
		struct GetConstOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			Symbol *name;
			
			GetConstOp(Tree::Variable *var, Tree::Variable *obj, Symbol *name) : Opcode(GetConst), var(var), obj(obj), name(name) {}
		};
		
		struct SetConstOp:
			public Opcode
		{
			Tree::Variable *obj;
			Symbol *name;
			Tree::Variable *var;
			
			SetConstOp(Tree::Variable *obj, Symbol *name, Tree::Variable *var) : Opcode(SetConst), obj(obj), name(name), var(var) {}
		};
		
		struct BranchIfOp:
			public Opcode
		{
			BasicBlock *ltrue;
			Tree::Variable *var;
			
			BranchIfOp(BasicBlock *ltrue, Tree::Variable *var) : Opcode(BranchIf), ltrue(ltrue), var(var) {}
		};
		
		struct BranchUnlessOp:
			public Opcode
		{
			BasicBlock *lfalse;
			Tree::Variable *var;
			
			BranchUnlessOp(BasicBlock *lfalse, Tree::Variable *var) : Opcode(BranchUnless), lfalse(lfalse), var(var) {}
		};
		
		struct BranchOp:
			public Opcode
		{
			BasicBlock *label;
			
			BranchOp(BasicBlock *label) : Opcode(Branch), label(label) {}
		};
		
		struct HandlerOp:
			public Opcode
		{
			size_t id;
			
			HandlerOp(size_t id) : Opcode(Handler), id(id) {}
		};
		
		struct UnwindOp:
			public Opcode
		{
			UnwindOp() : Opcode(Unwind) {}
		};
		
		struct UnwindReturnOp:
			public Opcode
		{
			Tree::Variable *var;
			void *code;
			
			UnwindReturnOp(Tree::Variable *var, void *code) : Opcode(UnwindReturn), var(var), code(code) {}
		};
		
		struct UnwindBreakOp:
			public Opcode
		{
			Tree::Variable *var;
			void *code;
			size_t index;
			
			UnwindBreakOp(Tree::Variable *var, void *code, size_t index) : Opcode(UnwindBreak), var(var), code(code), index(index) {}
		};
		
		struct BreakTargetOp:
			public Opcode
		{
			BreakTargetOp() : Opcode(BreakTarget) {}
		};
		
		struct ArrayOp:
			public Opcode
		{
			Tree::Variable *var;
			size_t element_count;
			
			ArrayOp(Tree::Variable *var, size_t element_count) : Opcode(Array), var(var), element_count(element_count) {}
		};
		
		struct StringOp:
			public Opcode
		{
			Tree::Variable *var;
			const char_t *str;
			
			StringOp(Tree::Variable *var, const char_t *str) : Opcode(String), var(var), str(str) {}
		};
		
		struct InterpolateOp:
			public Opcode
		{
			Tree::Variable *var;
			size_t param_count;
			
			InterpolateOp(Tree::Variable *var, size_t param_count) : Opcode(Interpolate), var(var), param_count(param_count) {}
		};
	};
};
