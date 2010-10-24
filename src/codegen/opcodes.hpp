#pragma once
#include "../../runtime/runtime.hpp"
#include "../common.hpp"
#include "../simple-list.hpp"

namespace Mirb
{
	class MemoryPool;
	
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class Block;
		
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
				BranchIf,
				BranchUnless,
				Branch,
				Return,
				Label,
				Handler,
				Unwind,
				UnwindReturn,
				UnwindBreak,
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
			Block *block;
			size_t scope_count;
			
			ClosureOp(Tree::Variable *var, Block *block, size_t scope_count) : Opcode(Closure), var(var), block(block), scope_count(scope_count) {}
		};
		
		struct ClassOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			rt_value name;
			Tree::Variable *super;
			Block *block;
			
			ClassOp(Tree::Variable *var, Tree::Variable *self, rt_value name, Tree::Variable *super, Block *block) : Opcode(Class), var(var), self(self), name(name), super(super), block(block) {}
		};
		
		struct ModuleOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			rt_value name;
			Block *block;
			
			ModuleOp(Tree::Variable *var, Tree::Variable *self, rt_value name, Block *block) : Opcode(Module), var(var), self(self), name(name), block(block) {}
		};
		
		struct MethodOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			rt_value name;
			Block *block;
			
			MethodOp(Tree::Variable *var, Tree::Variable *self, rt_value name, Block *block) : Opcode(Method), var(var), self(self), name(name), block(block) {}
		};
		
		struct CallOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			rt_value method;
			size_t param_count;
			Tree::Variable *block;
			
			CallOp(Tree::Variable *var, Tree::Variable *obj, rt_value method, size_t param_count, Tree::Variable *block) : Opcode(Call), var(var), obj(obj), method(method), param_count(param_count), block(block) {}
		};
		
		struct SuperOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			Tree::Variable *method;
			size_t param_count;
			Tree::Variable *block;
			
			SuperOp(Tree::Variable *var, Tree::Variable *obj, Tree::Variable *method, size_t param_count, Tree::Variable *block) : Opcode(Super), var(var), obj(obj), method(method), param_count(param_count), block(block) {}
		};
		
		struct GetIVarOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *self;
			rt_value name;
			
			GetIVarOp(Tree::Variable *var, Tree::Variable *self, rt_value name) : Opcode(GetIVar), var(var), self(self), name(name) {}
		};
		
		struct SetIVarOp:
			public Opcode
		{
			Tree::Variable *self;
			rt_value name;
			Tree::Variable *var;
			
			SetIVarOp(Tree::Variable *self, rt_value name, Tree::Variable *var) : Opcode(SetIVar), self(self), name(name), var(var) {}
		};
		
		struct GetConstOp:
			public Opcode
		{
			Tree::Variable *var;
			Tree::Variable *obj;
			rt_value name;
			
			GetConstOp(Tree::Variable *var, Tree::Variable *obj, rt_value name) : Opcode(GetConst), var(var), obj(obj), name(name) {}
		};
		
		struct SetConstOp:
			public Opcode
		{
			Tree::Variable *obj;
			rt_value name;
			Tree::Variable *var;
			
			SetConstOp(Tree::Variable *obj, rt_value name, Tree::Variable *var) : Opcode(SetConst), obj(obj), name(name), var(var) {}
		};
		
		struct BranchIfOp:
			public Opcode
		{
			class Label *ltrue; // TODO: more WTF
			Tree::Variable *var;
			
			BranchIfOp(class Label *ltrue, Tree::Variable *var) : Opcode(BranchIf), ltrue(ltrue), var(var) {}
		};
		
		struct BranchUnlessOp:
			public Opcode
		{
			class Label *lfalse;
			Tree::Variable *var;
			
			BranchUnlessOp(class Label *lfalse, Tree::Variable *var) : Opcode(BranchUnless), lfalse(lfalse), var(var) {}
		};
		
		struct BranchOp:
			public Opcode
		{
			class Label *label;
			
			BranchOp(class Label *label) : Opcode(Branch), label(label) {}
		};
		
		struct ReturnOp:
			public Opcode
		{
			Tree::Variable *var;
			
			ReturnOp(Tree::Variable *var) : Opcode(Return), var(var) {}
		};
		
		struct Label:
			public Opcode
		{
			#ifdef DEBUG
				size_t id;
			#endif
			
			Label() : Opcode(Opcode::Label) {}
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
			const char *str;
			
			StringOp(Tree::Variable *var, const char *str) : Opcode(String), var(var), str(str) {}
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
