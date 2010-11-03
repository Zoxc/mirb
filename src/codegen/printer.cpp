#include "../tree/tree.hpp"
#include "printer.hpp"
#include "opcodes.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		std::string ByteCodePrinter::var(Tree::Variable *var)
		{
			if(!var)
				return "nil";
			
			std::stringstream result;
			
			switch(var->type)
			{
				case Tree::Variable::Temporary:
				{
					result << "%" << var->index;
					
					break;
				}
				
				case Tree::Variable::Local:
				{
					auto named_var = (Tree::NamedVariable *)var;
					
					result << "%" << named_var->name->get_string();
					
					break;
				}
				
				case Tree::Variable::Heap:
				{
					auto named_var = (Tree::NamedVariable *)var;
					
					result << "!" << named_var->name->get_string();
					
					break;
				}
				
				default:
					assert(0);
			}

			if(highlight == var)
				return "<font color='dodgerblue2'>" + result.str() + "</font>";
			else
				return result.str();
		}
		
		std::string ByteCodePrinter::raw(size_t imm)
		{
			std::stringstream result;
			
			result << imm;
			
			return result.str();
		}
		
		std::string ByteCodePrinter::imm(rt_value imm)
		{
			return std::string(rt_string_to_cstr(rt_inspect(imm)));
		}
		
		std::string ByteCodePrinter::label(BasicBlock *label)
		{
			std::stringstream result;
			
			result << "#";
			
			#ifdef DEBUG
				result << label->id;
			#else
				result << label;
			#endif
			
			return result.str();
		}
		
		std::string ByteCodePrinter::block(Block *block)
		{
			std::stringstream result;
			
			result << "block " << block;
			
			return result.str();
		}
		
		std::string ByteCodePrinter::opcode(Opcode *opcode)
		{
			switch(opcode->op())
			{
				case Opcode::Move:
				{
					auto op = (MoveOp *)opcode;
					
					return var(op->dst) + " = " + var(op->src);
				}
				
				case Opcode::Load:
				{
					auto op = (LoadOp *)opcode;
					
					return var(op->var) + " = " + imm(op->imm);
				}
				
				case Opcode::LoadRaw:
				{
					auto op = (LoadRawOp *)opcode;
					
					return var(op->var) + " = " + raw(op->imm);
				}
				
				case Opcode::Push:
				{
					auto op = (PushOp *)opcode;
					
					return "push " + var(op->var);
				}
				
				case Opcode::PushImmediate:
				{
					auto op = (PushImmediateOp *)opcode;
					
					return "push " + imm(op->imm);
				}
				
				case Opcode::PushRaw:
				{
					auto op = (PushRawOp *)opcode;
					
					return "push " + raw(op->imm);
				}
				
				case Opcode::PushScope:
				{
					auto op = (PushScopeOp *)opcode;
					
					return "push " + block(op->block);
				}
				
				case Opcode::Closure:
				{
					auto op = (ClosureOp *)opcode;
					
					return var(op->var) + " = closure " + var(op->self) + ", " + block(op->block) + ", " + raw(op->scope_count);
				}
				
				case Opcode::Class:
				{
					auto op = (ClassOp *)opcode;
					
					return var(op->var) + " = class " + var(op->self) + ", " + imm(op->name) + ", " + var(op->super) + ", " + block(op->block);
				}
				
				case Opcode::Module:
				{
					auto op = (ModuleOp *)opcode;
					
					return var(op->var) + " = module " + var(op->self) + ", " + imm(op->name) + ", " + block(op->block);
				}
				
				case Opcode::Method:
				{
					auto op = (MethodOp *)opcode;
					
					return "method " + var(op->self) + ", " + imm(op->name) + ", " + block(op->block);
				}
				
				case Opcode::Call:
				{
					auto op = (CallOp *)opcode;
					
					return var(op->var) + " = call " + var(op->obj) + ", " + imm(op->method) + ", " + raw(op->param_count) + ", " + var(op->block) + ", " + (op->break_id == Tree::InvokeNode::no_break_id ? "nil" : raw(op->break_id));
				}
				
				case Opcode::Super:
				{
					auto op = (SuperOp *)opcode;
					
					return var(op->var) + " = super " + var(op->self) + ", " + var(op->module) + ", " + var(op->method) + ", " + raw(op->param_count) + ", " + var(op->block) + ", " + (op->break_id == Tree::InvokeNode::no_break_id ? "nil" : raw(op->break_id));
				}
				
				case Opcode::GetIVar:
				{
					auto op = (GetIVarOp *)opcode;
					
					return var(op->var) + " = ivar " + var(op->self) + ", " + imm(op->name);
				}
				
				case Opcode::SetIVar:
				{
					auto op = (SetIVarOp *)opcode;
					
					return "ivar " + var(op->self) + ", " + imm(op->name) + ", " + var(op->var);
				}
				
				case Opcode::GetConst:
				{
					auto op = (GetConstOp *)opcode;
					
					return var(op->var) + " = const " + var(op->obj) + ", " + imm(op->name);
				}
				
				case Opcode::SetConst:
				{
					auto op = (SetConstOp *)opcode;
					
					return "const " + var(op->obj) + ", " + imm(op->name) + ", " + var(op->var);
				}
				
				case Opcode::BranchIf:
				{
					auto op = (BranchIfOp *)opcode;
					
					return "branch " + label(op->ltrue) + " if " + var(op->var);
				}
				
				case Opcode::BranchUnless:
				{
					auto op = (BranchUnlessOp *)opcode;
					
					return "branch " + label(op->lfalse) + " unless " + var(op->var);
				}
				
				case Opcode::Branch:
				{
					auto op = (BranchOp *)opcode;
					
					return "branch " + label(op->label);
				}
				
				case Opcode::Return:
				{
					auto op = (ReturnOp *)opcode;
					
					return "ret " + var(op->var);
				}
				
				case Opcode::Handler:
				{
					auto op = (HandlerOp *)opcode;
					
					return "handler " + raw(op->id);
				}
				
				case Opcode::Unwind:
				{
					return "unwind";
				}
				
				case Opcode::UnwindReturn:
				{
					auto op = (UnwindReturnOp *)opcode;
					
					return "ret " + var(op->var) + ", " + raw((size_t)op->code);
				}
				
				case Opcode::UnwindBreak:
				{
					auto op = (UnwindBreakOp *)opcode;
					
					return "break " + var(op->var) + ", " + raw((size_t)op->code) + ", " + raw(op->index);
				}
				
				case Opcode::Array:
				{
					auto op = (ArrayOp *)opcode;
					
					return var(op->var) + " = array " + raw(op->element_count);
				}
				
				case Opcode::String:
				{
					auto op = (StringOp *)opcode;
					
					return var(op->var) + " = string '" + (const char *)op->str + "'";
				}
				
				case Opcode::Interpolate:
				{
					auto op = (InterpolateOp *)opcode;
					
					return var(op->var) + " = interpolate " + raw(op->param_count);
				}
				
				default:
					return "<unknown>";
			}
		}
		
		std::string ByteCodePrinter::print_basic_block(BasicBlock *block)
		{
			std::stringstream result;
			result << label(block) << ":\n";
			
			for(auto i = block->opcodes.begin(); i; ++i)
				result << opcode(*i) << "\n";
			
			return result.str();
		}

		std::string ByteCodePrinter::print_block(Block *block)
		{
			std::stringstream result;
			result << ";\n; " << this->block(block) << "\n;\n";
			
			for(auto i = block->basic_blocks.begin(); i; ++i)
				result << print_basic_block(*i) << "\n";
			
			return result.str();
		}
	};
};
