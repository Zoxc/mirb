#include "../tree/tree.hpp"
#include "../runtime.hpp"
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
			
			if(block->scope && (block->scope->referenced_scopes.size() > 0) && (var == block->heap_array_var))
				result << "@heap_array";
			else if(block->scope && block->scope->heap_vars && (var == block->heap_var))
				result << "@heap";
			else if(var == block->self_var)
				result << "@self";
			else
				switch(var->type)
				{
					case Tree::Variable::Temporary:
					{
						#ifdef DEBUG
							if(var->group != 0)
							{
								result << "%" << var->group->index << "[" << (var->index - var->group->index - 1) << "]";
							}
							else
								result << "%" << var->index;
						#else
							result << "%" << var->index;
						#endif
											
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
		
		std::string ByteCodePrinter::imm(value_t imm)
		{
			return inspect_object(imm);
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
		
		std::string ByteCodePrinter::print_block(Mirb::Block *block)
		{
			if(!block)
				return "nil";
			
			std::stringstream result;
			
			result << "block " << block;
			
			return result.str();
		}
		
		std::string ByteCodePrinter::opcode(Opcode *opcode)
		{
			switch(opcode->op)
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
				
				case Opcode::LoadArg:
				{
					auto op = (LoadArgOp *)opcode;
					
					return var(op->var) + " = arg " + raw(op->arg);
				}
				
				case Opcode::Group:
				{
					auto op = (GroupOp *)opcode;
					
					return var(op->var) + " = group " + raw(op->address);
				}
				
				case Opcode::Closure:
				{
					auto op = (ClosureOp *)opcode;
					
					return var(op->var) + " = closure " + var(op->self) + ", " + var(op->name) + ", " + var(op->module) + ", " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Class:
				{
					auto op = (ClassOp *)opcode;
					
					return (op->var ? var(op->var) + " = " : "") + "class " + var(op->self) + ", " + imm(op->name) + ", " + var(op->super) + ", " + print_block(op->block);
				}
				
				case Opcode::Module:
				{
					auto op = (ModuleOp *)opcode;
					
					return (op->var ? var(op->var) + " = " : "") + "module " + var(op->self) + ", " + imm(op->name) + ", " + print_block(op->block);
				}
				
				case Opcode::Method:
				{
					auto op = (MethodOp *)opcode;
					
					return "method " + var(op->self) + ", " + imm(op->name) + ", " + print_block(op->block);
				}
				
				case Opcode::Call:
				{
					auto op = (CallOp *)opcode;
					
					return (op->var ? var(op->var) + " = " : "") + "call " + var(op->obj) + ", " + imm(op->method) + ", " + var(op->block_var) + ", " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Super:
				{
					auto op = (SuperOp *)opcode;
					
					return (op->var ? var(op->var) + " = " : "") + "super " + var(op->self) + ", " + var(op->module) + ", " + var(op->method) + ", " + var(op->block_var) + ", " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Lookup:
				{
					auto op = (LookupOp *)opcode;
					
					return var(op->var) + " = " + var(op->array_var) + "[" + raw(op->index) + "]";
				}
				
				case Opcode::CreateHeap:
				{
					auto op = (CreateHeapOp *)opcode;
					
					return var(op->var) + " = create_heap";
				}
				
				case Opcode::GetHeapVar:
				{
					auto op = (GetHeapVarOp *)opcode;
					
					return var(op->var) + " = heap_var " + var(op->heap) + ", " + var(op->index);
				}
				
				case Opcode::SetHeapVar:
				{
					auto op = (SetHeapVarOp *)opcode;
					
					return "heap_var " + var(op->heap) + ", " + var(op->index) + " = " + var(op->var);
				}
				
				case Opcode::GetIVar:
				{
					auto op = (GetIVarOp *)opcode;
					
					return var(op->var) + " = ivar " + var(op->self) + ", " + imm(op->name);
				}
				
				case Opcode::SetIVar:
				{
					auto op = (SetIVarOp *)opcode;
					
					return "ivar " + var(op->self) + ", " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::GetConst:
				{
					auto op = (GetConstOp *)opcode;
					
					return var(op->var) + " = const " + var(op->obj) + ", " + imm(op->name);
				}
				
				case Opcode::SetConst:
				{
					auto op = (SetConstOp *)opcode;
					
					return "const " + var(op->obj) + ", " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::BranchIf:
				{
					auto op = (BranchIfOp *)opcode;
					
					return "branch " + label(op->label) + " if " + var(op->var);
				}
				
				case Opcode::BranchUnless:
				{
					auto op = (BranchUnlessOp *)opcode;
					
					return "branch " + label(op->label) + " unless " + var(op->var);
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
					
					return "handler " + (op->id == (size_t)-1 ? "nil" : raw(op->id));
				}
				
				case Opcode::Flush:
				{
					return "flush";
				}
				
				case Opcode::Unwind:
				{
					return "unwind";
				}
				
				case Opcode::UnwindReturn:
				{
					auto op = (UnwindReturnOp *)opcode;
					
					return "ret " + var(op->var) + ", " + print_block(op->code);
				}
				
				case Opcode::UnwindBreak:
				{
					auto op = (UnwindBreakOp *)opcode;
					
					return "break " + var(op->var) + ", " + print_block(op->code) + ", " + raw(op->index);
				}
				
				case Opcode::Array:
				{
					auto op = (ArrayOp *)opcode;
					
					return var(op->var) + " = array " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::String:
				{
					auto op = (StringOp *)opcode;
					
					return var(op->var) + " = string '" + (const char *)op->str + "'";
				}
				
				case Opcode::Interpolate:
				{
					auto op = (InterpolateOp *)opcode;
					
					return var(op->var) + " = interpolate " + var(op->argv) + ", " + raw(op->argc);
				}

				case Opcode::StaticCall:
				{
					auto op = (StaticCallOp *)opcode;
					
					std::stringstream result;
					
					result << var(op->var) << " = static_call 0x" << op->function;

					for(size_t i = 0; i < op->arg_count; ++i)
						result << ", " << var(op->args[i]);
					
					return result.str();
				}
				
				case Opcode::Raise:
				{
					return "raise";
				}
				
				case Opcode::Prologue:
				{
					return "prologue";
				}
				
				default:
					return "<unknown>";
			}
		}
		
		std::string ByteCodePrinter::print_basic_block(BasicBlock *block)
		{
			std::stringstream result;
			result << label(block) << ":\n";
			
			for(auto i = block->opcodes.begin(); i != block->opcodes.end(); ++i)
				result << opcode(*i) << "\n";
			
			return result.str();
		}

		std::string ByteCodePrinter::print()
		{
			std::stringstream result;
			result << ";\n; " << print_block(block->final) << "\n;\n";
			
			for(auto i = block->basic_blocks.begin(); i != block->basic_blocks.end(); ++i)
				result << print_basic_block(*i) << "\n";
			
			return result.str();
		}
	};
};
