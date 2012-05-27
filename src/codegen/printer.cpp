#include "../tree/tree.hpp"
#include "../runtime.hpp"
#include "../classes/fixnum.hpp"
#include "printer.hpp"
#include "opcodes.hpp"
#include "bytecode.hpp"

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
		
		std::string ByteCodePrinter::imm(Symbol *imm)
		{
			return imm->get_string();
		};
				
		std::string ByteCodePrinter::var(var_t var)
		{
			if(var == no_var)
				return "nil";
			
			std::stringstream result;

			result << "%" << var;

			return result.str();
		}
		
		std::string ByteCodePrinter::raw(intptr_t imm)
		{
			std::stringstream result;
			
			result << imm;
			
			return result.str();
		}
		
		std::string ByteCodePrinter::raw(size_t imm)
		{
			std::stringstream result;
			
			result << imm;
			
			return result.str();
		}
		
		std::string ByteCodePrinter::label(Label *label)
		{
			if(!label)
				return "error_label";

			std::stringstream result;

			result << "#";
			
			#ifdef MIRB_DEBUG_COMPILER
				result << label->id;
			#else
				result << label;
			#endif
			
			return result.str();
		}
		
		Label *ByteCodePrinter::get_label_reverse(const char *opcode)
		{
			for(auto i = bcg->branches.begin(); i != bcg->branches.end(); ++i)
			{
				if((*i).first == (size_t)(opcode - data))
				{
					return (*i).second;
				}
			}

			return nullptr;
		}
		
		Label *ByteCodePrinter::get_label(const char *opcode)
		{
			for(auto i = bcg->branches.begin(); i != bcg->branches.end(); ++i)
			{
				if((*i).second->pos == (size_t)(opcode - data))
				{
					return (*i).second;
				}
			}

			return nullptr;
		}
		
		std::string ByteCodePrinter::label(const char *opcode)
		{
			return label(get_label_reverse(opcode));
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
				
				case Opcode::LoadFloat:
				{
					auto op = (LoadFloatOp *)opcode;

					opcode += sizeof(LoadFloatOp);
					
					std::stringstream result;
					
					result << var(op->var) << " = " << op->value;
			
					return result.str();
				}
				
				case Opcode::LoadFixnum:
				{
					auto op = (LoadFixnumOp *)opcode;

					opcode += sizeof(LoadFixnumOp);
					
					return var(op->var) + " = " + raw(Fixnum::to_int(op->num));
				}
				
				case Opcode::LoadTrue:
				{
					auto op = (LoadTrueOp *)opcode;

					opcode += sizeof(LoadTrueOp);
					
					return var(op->var) + " = true";
				}
				
				case Opcode::LoadFalse:
				{
					auto op = (LoadFalseOp *)opcode;

					opcode += sizeof(LoadFalseOp);
					
					return var(op->var) + " = false";
				}
				
				case Opcode::LoadNil:
				{
					auto op = (LoadNilOp *)opcode;

					opcode += sizeof(LoadNilOp);
					
					return var(op->var) + " = nil";
				}
				
				case Opcode::LoadObject:
				{
					auto op = (LoadObjectOp *)opcode;

					opcode += sizeof(LoadObjectOp);
					
					return var(op->var) + " = Object";
				}
				
				case Opcode::LoadArg:
				{
					auto op = (LoadArgOp *)opcode;

					opcode += sizeof(LoadArgOp);
					
					return var(op->var) + " = arg " + raw(op->arg);
				}
				
				case Opcode::LoadArgFloat:
				{
					auto op = (LoadArgFloatOp *)opcode;

					opcode += sizeof(LoadArgFloatOp);
					
					return var(op->var) + " = arg_float " + raw(op->prev_reg) + ", " + raw(op->prev_def);
				}
				
				case Opcode::LoadArrayArg:
				{
					auto op = (LoadArrayArgOp *)opcode;

					opcode += sizeof(LoadArrayArgOp);
					
					return var(op->var) + " = array_arg " + raw(op->from_arg);
				}
				
				case Opcode::LoadArgBranch:
				{
					auto op = (LoadArgBranchOp *)opcode;
					
					result = var(op->var) + " = load_arg_branch " + label(opcode) + ", " + raw(op->arg) + ", " + raw(op->req_args);

					opcode += sizeof(LoadArgBranchOp);

					break;
				}
				
				case Opcode::LoadSymbol:
				{
					auto op = (LoadSymbolOp *)opcode;

					opcode += sizeof(LoadSymbolOp);
					
					return var(op->var) + " = :" + imm(op->symbol);
				}
				
				case Opcode::Assign:
				{
					auto op = (AssignOp *)opcode;

					opcode += sizeof(AssignOp);
					
					return var(op->var) + " = " + var(op->array) + "[" + raw(op->index) + "] if size > " + raw(op->size);
				}
				
				case Opcode::AssignArray:
				{
					auto op = (AssignArrayOp *)opcode;

					opcode += sizeof(AssignArrayOp);
					
					return var(op->var) + " = " + var(op->array) + "[" + raw(op->index) + "] if size > " + raw(op->size);
				}
				
				case Opcode::Push:
				{
					auto op = (PushOp *)opcode;

					opcode += sizeof(PushOp);
					
					return var(op->array) + " << " + var(op->value);
				}
				
				case Opcode::PushArray:
				{
					auto op = (PushArrayOp *)opcode;

					opcode += sizeof(PushArrayOp);
					
					return var(op->into) + " << *" + var(op->from);
				}
				
				case Opcode::AssertBlock:
				{
					auto op = (AssertBlockOp *)opcode;

					opcode += sizeof(AssertBlockOp);
					
					return "assert_block " + var(op->var);
				}
				
				case Opcode::Closure:
				{
					auto op = (ClosureOp *)opcode;

					opcode += sizeof(ClosureOp);
					
					return var(op->var) + " = closure " + print_block(op->block) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Class:
				{
					auto op = (ClassOp *)opcode;

					opcode += sizeof(ClassOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "class " + imm(op->name) + ", " + var(op->super) + ", " + print_block(op->block) + (op->scope != no_var ? ", " + var(op->scope) : "");
				}
				
				case Opcode::SingletonClass:
				{
					auto op = (SingletonClassOp *)opcode;

					opcode += sizeof(SingletonClassOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "singleton_class " + var(op->singleton) + ", " + print_block(op->block);
				}
				
				case Opcode::Module:
				{
					auto op = (ModuleOp *)opcode;

					opcode += sizeof(ModuleOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "module " + imm(op->name) + ", " + print_block(op->block) + (op->scope != no_var ? ", " + var(op->scope) : "");
				}
				
				case Opcode::Method:
				{
					auto op = (MethodOp *)opcode;

					opcode += sizeof(MethodOp);
					
					return "method " + imm(op->name) + ", " + print_block(op->block);
				}
				
				case Opcode::SingletonMethod:
				{
					auto op = (SingletonMethodOp *)opcode;

					opcode += sizeof(SingletonMethodOp);
					
					return "singleton_method " + var(op->singleton)+ ", " + imm(op->name) + ", " + print_block(op->block);
				}
				
				case Opcode::Alias:
				{
					auto op = (AliasOp *)opcode;

					opcode += sizeof(AliasOp);
					
					return "alias " + var(op->new_name)+ ", " + var(op->old_name);
				}
				
				case Opcode::Call:
				{
					auto op = (CallOp *)opcode;

					opcode += sizeof(CallOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "call " + var(op->obj) + ", " + imm(op->method) + ", " + var(op->block_var) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Super:
				{
					auto op = (SuperOp *)opcode;

					opcode += sizeof(SuperOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "super " + var(op->block_var) + ", " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::VariadicCall:
				{
					auto op = (VariadicCallOp *)opcode;

					opcode += sizeof(VariadicCallOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "call " + var(op->obj) + ", " + imm(op->method) + ", " + var(op->block_var) + ", *" + var(op->argv);
				}
				
				case Opcode::VariadicSuper:
				{
					auto op = (VariadicSuperOp *)opcode;

					opcode += sizeof(VariadicSuperOp);
					
					return (op->var != no_var ? var(op->var) + " = " : "") + "super " + var(op->block_var) + ", *" + var(op->argv);
				}
				
				case Opcode::Lookup:
				{
					auto op = (LookupOp *)opcode;

					opcode += sizeof(LookupOp);
					
					return var(op->var) + " = heaps [" + raw(op->index) + "]";
				}
				
				case Opcode::Self:
				{
					auto op = (SelfOp *)opcode;

					opcode += sizeof(SelfOp);
					
					return var(op->var) + " = self";
				}
				
				case Opcode::Block:
				{
					auto op = (BlockOp *)opcode;

					opcode += sizeof(BlockOp);
					
					return var(op->var) + " = block_arg";
				}
				
				case Opcode::CreateHeap:
				{
					auto op = (CreateHeapOp *)opcode;

					opcode += sizeof(CreateHeapOp);
					
					return var(op->var) + " = create_heap(" + raw(op->vars) + ")";
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
					
					return var(op->var) + " = ivar " + imm(op->name);
				}
				
				case Opcode::SetIVar:
				{
					auto op = (SetIVarOp *)opcode;

					opcode += sizeof(SetIVarOp);
					
					return "ivar " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::GetGlobal:
				{
					auto op = (GetGlobalOp *)opcode;

					opcode += sizeof(GetGlobalOp);
					
					return var(op->var) + " = global " + imm(op->name);
				}
				
				case Opcode::SetGlobal:
				{
					auto op = (SetGlobalOp *)opcode;

					opcode += sizeof(SetGlobalOp);
					
					return "global " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::GetScopedConst:
				{
					auto op = (GetScopedConstOp *)opcode;

					opcode += sizeof(GetScopedConstOp);
					
					return var(op->var) + " = const " + var(op->obj) + ", " + imm(op->name);
				}
				
				case Opcode::SetScopedConst:
				{
					auto op = (SetScopedConstOp *)opcode;

					opcode += sizeof(SetScopedConstOp);
					
					return "const " + var(op->obj) + ", " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::GetConst:
				{
					auto op = (GetConstOp *)opcode;

					opcode += sizeof(GetConstOp);
					
					return var(op->var) + " = const " + imm(op->name);
				}
				
				case Opcode::SetConst:
				{
					auto op = (SetConstOp *)opcode;

					opcode += sizeof(SetConstOp);
					
					return "const " + imm(op->name) + " = " + var(op->var);
				}
				
				case Opcode::BranchIf:
				{
					auto op = (BranchIfOp *)opcode;
					
					result = "branch " + label(opcode) + " if " + var(op->var);

					opcode += sizeof(BranchIfOp);

					break;
				}
				
				case Opcode::BranchUnless:
				{
					auto op = (BranchUnlessOp *)opcode;
					
					result = "branch " + label(opcode) + " unless " + var(op->var);

					opcode += sizeof(BranchUnlessOp);

					break;
				}
				
				case Opcode::Branch:
				{
					result = "branch " + label(opcode);

					opcode += sizeof(BranchOp);

					break;
				}
				
				case Opcode::Return:
				{
					auto op = (ReturnOp *)opcode;

					opcode += sizeof(ReturnOp);
					
					return "ret " + var(op->var);
				}
				
				case Opcode::Handler:
				{
					auto op = (HandlerOp *)opcode;

					opcode += sizeof(HandlerOp);
					
					return "handler " + (op->block ? raw((size_t)op->block) : "nil");
				}
				
				case Opcode::UnwindEnsure:
				{
					opcode += sizeof(UnwindEnsureOp);

					return "unwind_ensure";
				}
				
				case Opcode::UnwindFilter:
				{
					opcode += sizeof(UnwindFilterOp);

					return "unwind_filter";
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
					
					return "break " + var(op->var) + ", " + print_block(op->code) + ", " + var(op->parent_dst);
				}
				
				case Opcode::UnwindNext:
				{
					auto op = (UnwindNextOp *)opcode;

					opcode += sizeof(UnwindNextOp);
					
					return "next " + var(op->var);
				}
				
				case Opcode::UnwindRedo:
				{
					opcode += sizeof(UnwindRedoOp);
					
					return "redo " + label(opcode);
				}
				
				case Opcode::Array:
				{
					auto op = (ArrayOp *)opcode;

					opcode += sizeof(ArrayOp);
					
					return var(op->var) + " = array " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::Hash:
				{
					auto op = (HashOp *)opcode;

					opcode += sizeof(HashOp);
					
					return var(op->var) + " = hash " + var(op->argv) + ", " + raw(op->argc);
				}
				
				case Opcode::String:
				{
					auto op = (StringOp *)opcode;

					opcode += sizeof(StringOp);
					
					return var(op->var) + " = string '" + std::string((const char *)op->str.data, op->str.length) + "'";
				}
				
				case Opcode::Regexp:
				{
					auto op = (RegexpOp *)opcode;

					opcode += sizeof(RegexpOp);
					
					return var(op->var) + " = regexp /" + std::string((const char *)op->str.data, op->str.length) + "/";
				}
				
				case Opcode::Range:
				{
					auto op = (RangeOp *)opcode;

					opcode += sizeof(RangeOp);
					
					return var(op->var) + " = range " + var(op->low) + (op->exclusive ? "..." : "..") + var(op->high);
				}
				
				case Opcode::Interpolate:
				{
					auto op = (InterpolateOp *)opcode;

					opcode += sizeof(InterpolateOp);
					
					return var(op->var) + " = interpolate " + var(op->argv) + ", " + raw(op->argc) + ", " + raw(op->result);
				}

				default:
					mirb_runtime_abort("Unknown opcode");
			}

			return result;
		}
		
		std::string ByteCodePrinter::print()
		{
			std::stringstream result;
			
			result << ";\n; " << print_block(bcg->final) << "\n;\n";

			char *opcodes = (char *)bcg->opcode.compact<Prelude::Allocator::Standard>();
			
			data = opcodes;
			
			for(const char *c = opcodes; c != opcodes + bcg->opcode.size();)
			{
				Label *l = get_label(c);

				if(l)
					result<< "\n" << label(l) << ":\n";

				result << opcode(c) << "\n";
			}
			
			Prelude::Allocator::Standard::free(opcodes);
			
			return result.str();
		}
	};
};
