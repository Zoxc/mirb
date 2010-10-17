#include "printer.hpp"
#include "node.hpp"
#include "nodes.hpp"
#include "../symbol-pool.hpp"
#include "../../compiler/block.hpp"

namespace Mirb
{
	template<class T> std::string Printer::join(T &list, std::string seperator)
	{
		std::string result;

		for(auto i = list.begin(); i; i++)
		{
			result += print_node(*i);

			if((*i)->entry.next)
				result += seperator;
		}

		return result;
	}

	template<class T> std::string Printer::join(T &list, std::string pre, std::string post)
	{
		std::string result;

		for(auto i = list.begin(); i; i++)
			result += pre + print_node(*i) + post;

		return result;
	}
	
	std::string Printer::get_indent(size_t indent)
	{
		std::string result;

		for(size_t i = 0; i < indent; i++)
			result += "\t";

		return result;
	}
	
	std::string Printer::print_symbol(Symbol *symbol)
	{
		if(symbol)
			return symbol->get_string();
		else
			return "<null_symbol>";
	}
	
	std::string Printer::wrap(SimpleNode *node, std::string result)
	{
		return result;
	}
	
	std::string Printer::print_node(SimpleNode *node)
	{
		return wrap(node, this->node(node, 0));
	}
	
	std::string Printer::print_node(SimpleNode *node, size_t indent)
	{
		return wrap(node, this->node(node, indent));
	}
	
	std::string Printer::node(SimpleNode *node, size_t indent)
	{
		if(!node)
			return "<null_node>";

		switch(node->type())
		{
			case SimpleNode::BinaryOp:
			case SimpleNode::Assignment:
			{
				BinaryOpNode *target = (BinaryOpNode *)node;
				
				return print_node(target->left) + " " + Lexeme::names[target->op] + " " + print_node(target->right);
			}
			
			case SimpleNode::UnaryOp:
			{
				UnaryOpNode *target = (UnaryOpNode *)node;
				
				return Lexeme::names[target->op] +  " " + print_node(target->value);
			}
			
			case SimpleNode::String:
			{
				StringNode *target = (StringNode *)node;

				return std::string((const char *)target->string);
			}
			
			case SimpleNode::InterpolatedPair:
			{
				InterpolatedPairNode *target = (InterpolatedPairNode *)node;
				
				return std::string((const char *)target->string) + "#{" + join(target->statements, "; ");
			}
			
			case SimpleNode::InterpolatedString:
			{
				InterpolatedStringNode *target = (InterpolatedStringNode *)node;
				
				return "\"" + join(target->pairs, "}") + "}" + std::string((const char *)target->tail) + "\"";
			}
			
			case SimpleNode::Integer:
			{
				IntegerNode *target = (IntegerNode *)node;

				std::stringstream out;
				out << target->value;
				return out.str();
			}
			
			case SimpleNode::Variable:
			{
				VariableNode *target = (VariableNode *)node;
				
				switch(target->variable_type)
				{
					case VariableNode::Temporary:
					{
						std::stringstream out;
						out << "__temp" << target->var->index << "__";
						return out.str();
					}
						
					case VariableNode::Local:
						return print_symbol((Symbol *)target->var->name);
					
					case VariableNode::Instance:
						return print_symbol(target->ivar.name);
					
					case VariableNode::Constant:
						return print_node(target->constant.left) + "::" + print_symbol(target->constant.name);
						
					default:
						return "<error>";
				};
			}
			
			case SimpleNode::Self:
			{
				return "self";
			}

			case SimpleNode::Nil:
			{
				return "nil";
			}

			case SimpleNode::True:
			{
				return "true";
			}

			case SimpleNode::False:
			{
				return "false";
			}
			
			case SimpleNode::Group:
			{
				GroupNode *target = (GroupNode *)node;
				
				std::string result;
				
				for(auto i = target->statements.begin(); i; i++)
					result += get_indent(indent) + print_node(*i, indent + 1) + "\n";

				return result;
			}
			
			case SimpleNode::Block:
			{
				BlockNode *target = (BlockNode *)node;
				return "do\n" + print_node(target->scope.group) + "end\n";
			}

			case SimpleNode::Call:
			{
				CallNode *target = (CallNode *)node;
				
				std::string result = print_node(target->object) + "." + print_symbol(target->method);
				
				if(!target->arguments.empty())
					result += "(" + join(target->arguments, ", ") + ")";
					
				if(target->block)
					result += " " + print_node(target->block);
					
				return result;
			}

			case SimpleNode::Super:
			{
				SuperNode *target = (SuperNode *)node;
				
				std::string result = "super";
				
				if(!target->arguments.empty())
					result += "(" + join(target->arguments, ", ") + ")";
				
				if(target->block)
					result += " " + print_node(target->block);
				
				return result;
			}

			case SimpleNode::BreakHandler:
			{
				BreakHandlerNode *target = (BreakHandlerNode *)node;
				
				return print_node(target->code);
			}
			
			case SimpleNode::If:
			{
				IfNode *target = (IfNode *)node;
				
				return (target->inverted ? "unless " : "if ") + print_node(target->left) + "\n" + print_node(target->middle) + "\nelse\n" + print_node(target->right) + "\nend\n";
			}
			
			case SimpleNode::Rescue:
			{
				RescueNode *target = (RescueNode *)node;
				
				return "rescue\n" + print_node(target->group);
			}
			
			case SimpleNode::Handler:
			{
				HandlerNode *target = (HandlerNode *)node;
				
				std::string result = "begin\n" + print_node(target->code) + join(target->rescues);
				
				if(target->group)
					result += "ensure\n" + print_node(target->group);
				
				return result + "\nend\n";
			}
			
			case SimpleNode::Return:
			{
				ReturnNode *target = (ReturnNode *)node;
				
				return "return " + print_node(target->value);
			}
			
			case SimpleNode::Next:
			{
				NextNode *target = (NextNode *)node;
				
				return "return " + print_node(target->value);
			}
			
			case SimpleNode::Break:
			{
				BreakNode *target = (BreakNode *)node;
				
				return "return " + print_node(target->value);
			}
			
			case SimpleNode::Redo:
			{
				return "redo";
			}
			
			case SimpleNode::Module:
			{
				ModuleNode *target = (ModuleNode *)node;
				
				return "module " + print_symbol(target->name) + "\n" + print_node(target->scope.group) + "end\n";
			}
			
			case SimpleNode::Class:
			{
				ClassNode *target = (ClassNode *)node;
				
				return "class " + print_symbol(target->name) + " < " + print_node(target->super) + "\n" + print_node(target->scope.group) + "end\n";
			}
			
			case SimpleNode::Method:
			{
				MethodNode *target = (MethodNode *)node;
				
				return "def " + print_symbol(target->name) + "\n" + print_node(target->scope.group) + "end\n";
			}
			
			default:
				return "<unknown>";
		}
	}
	
	std::string DebugPrinter::wrap(SimpleNode *node, std::string result)
	{
		if(node)
		{
			return "(" + SimpleNode::names[node->type()] + ": " +  result + ")";
		}
		else
			return result;
	}
};
