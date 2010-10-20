#pragma once
#include "../../runtime/runtime.hpp"
#include "../common.hpp"
#include "../simple-list.hpp"

namespace Mirb
{
	class Block;
	class Variable;
	
	struct Opcode;
	
	class OpcodeBlock
	{
			static const size_t block_size = 0x256;
			
			Opcode *block_start;
			unsigned char *block_current;
			
			SimpleEntry<OpcodeBlock> entry;
			
			static OpcodeBlock *allocate();
	};
	
	struct Opcode
	{
		enum Type
		{
			EndBlock, // This indicates the end of this block of opcodes
			
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
		
		Opcode(Type op) : op(op) {}
	};
	
	struct LabelOp;
	
	struct MoveOp:
		public Opcode
	{
		Variable *dst;
		Variable *src;
		
		MoveOp(Variable *dst, Variable *src) : Opcode(Move), dst(dst), src(src) {}
	};
	
	struct LoadOp:
		public Opcode
	{
		Variable *var;
		rt_value imm;
		
		LoadOp(Variable *var, rt_value imm) : Opcode(Load), var(var), imm(imm) {}
	};
	
	struct LoadRawOp:
		public Opcode
	{
		Variable *var;
		size_t imm;
		
		LoadRawOp(Variable *var, size_t imm) : Opcode(LoadRaw), var(var), imm(imm) {}
	};
	
	struct PushOp:
		public Opcode
	{
		Variable *var;
		
		PushOp(Variable *var) : Opcode(Push), var(var) {}
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
		Variable *var;
		Block *block;
		
		ClosureOp(Variable *var, Block *block) : Opcode(Closure), var(var), block(block) {}
	};
	
	struct ClassOp:
		public Opcode
	{
		Variable *var;
		Variable *self;
		rt_value name;
		Variable *super;
		Block *block;
		
		ClassOp(Variable *var, Variable *self, rt_value name, Variable *super, Block *block) : Opcode(Class), var(var), self(self), name(name), super(super), block(block) {}
	};
	
	struct ModuleOp:
		public Opcode
	{
		Variable *var;
		Variable *self;
		rt_value name;
		Block *block;
		
		ModuleOp(Variable *var, Variable *self, rt_value name, Block *block) : Opcode(Module), var(var), self(self), name(name), block(block) {}
	};
	
	struct MethodOp:
		public Opcode
	{
		Variable *var;
		Variable *self;
		rt_value name;
		Block *block;
		
		MethodOp(Variable *var, Variable *self, rt_value name, Block *block) : Opcode(Method), var(var), self(self), name(name), block(block) {}
	};
	
	struct CallOp:
		public Opcode
	{
		Variable *var;
		rt_value method;
		size_t param_count;
		Variable *block;
		
		CallOp(Variable *var, rt_value method, size_t param_count, Variable *block) : Opcode(Call), var(var), method(method), param_count(param_count), block(block) {}
	};
	
	struct SuperOp:
		public Opcode
	{
		Variable *var;
		Variable *method;
		size_t param_count;
		Variable *block;
		
		SuperOp(Variable *var, Variable *method, size_t param_count, Variable *block) : Opcode(Super), var(var), method(method), param_count(param_count), block(block) {}
	};
	
	struct GetIVarOp:
		public Opcode
	{
		Variable *var;
		Variable *self;
		rt_value name;
		
		GetIVarOp(Variable *var, Variable *self, rt_value name) : Opcode(GetIVar), var(var), self(self), name(name) {}
	};
	
	struct SetIVarOp:
		public Opcode
	{
		Variable *self;
		rt_value name;
		Variable *var;
		
		SetIVarOp(Variable *self, rt_value name, Variable *var) : Opcode(SetIVar), self(self), name(name), var(var) {}
	};
	
	struct GetConstOp:
		public Opcode
	{
		Variable *var;
		Variable *obj;
		rt_value name;
		
		GetConstOp(Variable *var, Variable *obj, rt_value name) : Opcode(GetConst), var(var), obj(obj), name(name) {}
	};
	
	struct SetConstOp:
		public Opcode
	{
		Variable *obj;
		rt_value name;
		Variable *var;
		
		SetConstOp(Variable *obj, rt_value name, Variable *var) : Opcode(SetConst), obj(obj), name(name), var(var) {}
	};
	
	struct BranchIfOp:
		public Opcode
	{
		LabelOp *ltrue;
		Variable *var;
		
		BranchIfOp(LabelOp *ltrue, Variable *var) : Opcode(BranchIf), ltrue(ltrue), var(var) {}
	};
	
	struct BranchUnlessOp:
		public Opcode
	{
		LabelOp *lfalse;
		Variable *var;
		
		BranchUnlessOp(LabelOp *lfalse, Variable *var) : Opcode(BranchUnless), lfalse(lfalse), var(var) {}
	};
	
	struct BranchOp:
		public Opcode
	{
		LabelOp *label;
		
		BranchOp(LabelOp *label) : Opcode(Branch), label(label) {}
	};
	
	struct ReturnOp:
		public Opcode
	{
		Variable *var;
		
		ReturnOp(Variable *var) : Opcode(Return), var(var) {}
	};
	
	struct LabelOp:
		public Opcode
	{
		LabelOp() : Opcode(Label) {}
	};
	
	struct UnwindOp:
		public Opcode
	{
		UnwindOp() : Opcode(Unwind) {}
	};
	
	struct UnwindReturnOp:
		public Opcode
	{
		Variable *var;
		rt_value code;
		
		UnwindReturnOp(Variable *var, rt_value code) : Opcode(UnwindReturn), var(var), code(code) {}
	};
	
	struct UnwindBreakOp:
		public Opcode
	{
		Variable *var;
		rt_value code;
		
		UnwindBreakOp(Variable *var, rt_value code) : Opcode(UnwindBreak), var(var), code(code) {}
	};
	
	struct ArrayOp:
		public Opcode
	{
		Variable *var;
		size_t element_count;
		
		ArrayOp(Variable *var, size_t element_count) : Opcode(Array), var(var), element_count(element_count) {}
	};
	
	struct StringOp:
		public Opcode
	{
		Variable *var;
		const char *str;
		
		StringOp(Variable *var, const char *str) : Opcode(String), var(var), str(str) {}
	};
	
	struct InterpolateOp:
		public Opcode
	{
		Variable *var;
		size_t param_count;
		
		InterpolateOp(Variable *var, size_t param_count) : Opcode(Interpolate), var(var), param_count(param_count) {}
	};
};
