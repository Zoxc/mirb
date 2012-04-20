#include "../tree/tree.hpp"
#include "../runtime.hpp"
#include "printer.hpp"
#include "opcodes.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		std::string ByteCodePrinter::var_name(Tree::NamedVariable *var)
		{
			if(var->name)
				return var->name->get_string();
			else
			{
				std::stringstream result;

				result << "0x" << var;

				return result.str();
			}
		}

		std::string ByteCodePrinter::var(var_t var)
		{
			if(var == -1)
				return "nil";
			
			std::stringstream result;

			result << "%" << var;

			return result.str();/*
			
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
					
						result << "%" << var_name(named_var);
					
						break;
					}
				
					case Tree::Variable::Heap:
					{
						auto named_var = (Tree::NamedVariable *)var;
					
						result << "!" << var_name(named_var);
					
						break;
					}
				
					default:
						assert(0);
				}

			if(highlight == var)
				return "<font color='dodgerblue2'>" + result.str() + "</font>";
			else
				return result.str();*/
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
			if(!label)
				return "error_label";

			std::stringstream result;

			result << "#";
			
			#ifdef DEBUG
				result << label->id;
			#else
				result << label;
			#endif
			
			return result.str();
		}
		
		std::string ByteCodePrinter::label(const char *opcode)
		{
			BasicBlock *block = 0;

			for(auto i = basic_block->branches.begin(); i != basic_block->branches.end(); ++i)
			{
				if((*i).first == (opcode - data))
				{
					block = (*i).second;
					break;
				}
			}

			return label(block);
		}
		
		std::string ByteCodePrinter::print_block(Mirb::Block *block)
		{
			if(!block)
				return "nil";
			
			std::stringstream result;
			
			result << "block " << block;
			
			return result.str();
		}
		
		std::string ByteCodePrinter::opcode(const char *&opcode)
		{
			std::string result;

			switch(*opcode)
			{
				case Opcode::Move:
				{
					auto op = (MoveOp *)opcode;

					opcode += sizeof(MoveOp);
					
					return var(op->dst) + " = " + var(op->src);
				}
				
				case Opcode::Load:
				{
					auto op = (LoadOp *)opcode;

					opcode += sizeof(LoadOp);
					
					return var(op->var) + " = " + imm(op->imm);
				}
				
				case Opcode::LoadRaw:
				{
					auto op = (LoadRawOp *)opcode;

					opcode += sizeof(LoadRawOp);
					
					return var(op->var) + " = " + raw(op->imm);
				}
				
				case Opcode::LoadArg:
				{
					auto op = (LoadArgOp *)opcode;

					opcode += sizeof(LoadArgOp);
					
					return var(op->var) + " = arg " + raw(op->arg);
				}
				
				case Opcode::Group:
				{
					auto op = (GroupOp *)opcode;

					opcode += sizeof(GroupOp);
					
					return var(op->var) + " = group " + raw(op->address);
				}
				
				case Opcode::Closure:
				{
					auto op = (ClosureOp *)opcode;

					opcode += sizeof(ClosureOp);
					
					return var(op->var) + " = closure " + var(op->self) + ", " + var(op->name) + ", " + var(op->module) + ", " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Class:
				{
					auto op = (ClassOp *)opcode;

					opcode += sizeof(ClassOp);
					
					return (op->var ? var(op->var) + " = " : "") + "class " + var(op->self) + ", " + imm(op->name) + ", " + var(op->super) + ", " + print_block(op->block);
				}
				
				case Opcode::Module:
				{
					auto op = (ModuleOp *)opcode;

					opcode += sizeof(ModuleOp);
					
					return (op->var ? var(op->var) + " = " : "") + "module " + var(op->self) + ", " + imm(op->name) + ", " + print_block(op->block);
				}
				
				case Opcode::Method:
				{
					auto op = (MethodOp *)opcode;

					opcode += sizeof(MethodOp);
					
					return "method " + var(op->self) + ", " + imm(op->name) + ", " + print_block(op->block);
				}
				
				case Opcode::Call:
				{
					auto op = (CallOp *)opcode;

					opcode += sizeof(CallOp);
					
					return (op->var ? var(op->var) + " = " : "") + "call " + var(op->obj) + ", " + imm(op->method) + ", " + var(op->block_var) + ", " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Super:
				{
					auto op = (SuperOp *)opcode;

					opcode += sizeof(SuperOp);
					
					return (op->var ? var(op->var) + " = " : "") + "super " + var(op->self) + ", " + var(op->module) + ", " + var(op->method) + ", " + var(op->block_var) + ", " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Lookup:
				{
					auto op = (LookupOp *)opcode;

					opcode += sizeof(LookupOp);
					
					return var(op->var) + " = " + var(op->array_var) + "[" + raw(op->index) + "]";
				}
				
				case Opcode::CreateHeap:
				{
					auto op = (CreateHeapOp *)opcode;

					opcode += sizeof(CreateHeapOp);
					
					return var(op->var) + " = create_heap";
				}
				
				case Opcode::GetHeapVar:
				{
					auto op = (GetHeapVarOp *)opcode;

					opcode += sizeof(GetHeapVarOp);
					
					return var(op->var) + " = heap_var " + var(op->heap) + ", " + var(op->index);
				}
				
				case Opcode::SetHeapVar:
				{
					auto op = (SetHeapVarOp *)opcode;

					opcode += sizeof(SetHeapVarOp);
					
					return "heap_var " + var(op->heap) + ", " + var(op->index) + " = " + var(op->var);
				}
				
				case Opcode::GetIVar:
				{
					auto op = (GetIVarOp *)opcode;

					opcode += sizeof(GetIVarOp);
					
					return var(op->var) + " = ivar " + var(op->self) + ", " + imm(op->name);
				}
				
				case Opcode::SetIVar:
				{
					auto op = (SetIVarOp *)opcode;

					opcode += sizeof(SetIVarOp);
					
					return "ivar " + var(op->self) + ", " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::GetConst:
				{
					auto op = (GetConstOp *)opcode;

					opcode += sizeof(GetConstOp);
					
					return var(op->var) + " = const " + var(op->obj) + ", " + imm(op->name);
				}
				
				case Opcode::SetConst:
				{
					auto op = (SetConstOp *)opcode;

					opcode += sizeof(SetConstOp);
					
					return "const " + var(op->obj) + ", " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::BranchIf:
				{
					auto op = (BranchIfOp *)opcode;
					
					result = "branch " + label(opcode) + " if " + var(op->var);

					opcode += sizeof(BranchIfOp);

					break;
				}
				
				case Opcode::BranchIfZero:
				{
					auto op = (BranchIfZeroOp *)opcode;
					
					result = "branch " + label(opcode) + " if_zero " + var(op->var);

					opcode += sizeof(BranchIfZeroOp);

					break;
				}
				
				case Opcode::BranchUnless:
				{
					auto op = (BranchUnlessOp *)opcode;
					
					result = "branch " + label(opcode) + " unless " + var(op->var);

					opcode += sizeof(BranchUnlessOp);

					break;
				}
				
				case Opcode::BranchUnlessZero:
				{
					auto op = (BranchUnlessZeroOp *)opcode;
					
					result = "branch " + label(opcode) + " unless_zero " + var(op->var);

					opcode += sizeof(BranchUnlessZeroOp);

					break;
				}
				
				case Opcode::Branch:
				{
					auto op = (BranchOp *)opcode;
					
					result = "branch " + label(opcode);

					opcode += sizeof(BranchOp);

					break;
				}
				
				case Opcode::Return:
				{
					auto op = (ReturnOp *)opcode;

					opcode += sizeof(ReturnOp);
					
					return "ret " + var(block->final->return_var);
				}
				
				case Opcode::Handler:
				{
					auto op = (HandlerOp *)opcode;

					opcode += sizeof(HandlerOp);
					
					return "handler " + (op->id == (size_t)-1 ? "nil" : raw(op->id));
				}
				
				case Opcode::Unwind:
				{
					opcode += sizeof(UnwindOp);

					return "unwind";
				}
				
				case Opcode::UnwindReturn:
				{
					auto op = (UnwindReturnOp *)opcode;

					opcode += sizeof(UnwindReturnOp);
					
					return "ret " + var(op->var) + ", " + print_block(op->code);
				}
				
				case Opcode::UnwindBreak:
				{
					auto op = (UnwindBreakOp *)opcode;

					opcode += sizeof(UnwindBreakOp);
					
					return "break " + var(op->var) + ", " + print_block(op->code) + ", " + raw(op->index);
				}
				
				case Opcode::Array:
				{
					auto op = (ArrayOp *)opcode;

					opcode += sizeof(ArrayOp);
					
					return var(op->var) + " = array " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::String:
				{
					auto op = (StringOp *)opcode;

					opcode += sizeof(StringOp);
					
					return var(op->var) + " = string '" + (const char *)op->str + "'";
				}
				
				case Opcode::Interpolate:
				{
					auto op = (InterpolateOp *)opcode;

					opcode += sizeof(InterpolateOp);
					
					return var(op->var) + " = interpolate " + var(op->argv) + ", " + raw(op->argc);
				}

				case Opcode::StaticCall:
				{
					auto op = (StaticCallOp *)opcode;

					opcode += sizeof(StaticCallOp);
					
					std::stringstream result;
					
					result << var(op->var) << " = static_call 0x" << op->function;

					for(size_t i = 0; i < op->arg_count; ++i)
						result << ", " << var(op->args[i]);
					
					return result.str();
				}
				
				case Opcode::Raise:
				{
					opcode += sizeof(RaiseOp);

					return "raise";
				}
				
				case Opcode::SetupVars:
				{
					auto op = (SetupVarsOp *)opcode;

					opcode += sizeof(SetupVarsOp);
					
					std::stringstream result;
					
					result << "setup_vars";

					for(size_t i = 0; i < op->arg_count; ++i)
					{
						result << " " << var(op->args[i]);

						if(i < op->arg_count - 1)
							result << ",";
					}
					
					return result.str();
				}
				
				default:
					opcode += 1;

					return "<unknown>";
			}

			return result;
		}
		
		std::string ByteCodePrinter::print_basic_block(BasicBlock *block)
		{
			std::stringstream result;
			result << label(block) << ":\n";

			std::string code = block->opcodes.str();

			basic_block = block;
			data = code.data();
			
			for(const char *c = code.data(); c != code.data() + code.size();)
				result << opcode(c) << "\n";
			
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
