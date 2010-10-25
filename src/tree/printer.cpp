#include "printer.hpp"
#include "node.hpp"
#include "nodes.hpp"
#include "tree.hpp"
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
	
	std::string Printer::wrap(Tree::SimpleNode *node, std::string result)
	{
		return result;
	}
	
	std::string Printer::print_node(Tree::SimpleNode *node)
	{
		return wrap(node, this->node(node, 0));
	}
	
	std::string Printer::print_node(Tree::SimpleNode *node, size_t indent)
	{
		return wrap(node, this->node(node, indent));
	}
	
	std::string Printer::node(Tree::SimpleNode *node, size_t indent)
	{
		if(!node)
			return "<null_node>";

		switch(node->type())
		{
			case Tree::SimpleNode::BinaryOp:
			case Tree::SimpleNode::Assignment:
			{
				auto target = (Tree::BinaryOpNode *)node;
				
				return print_node(target->left) + " " + Lexeme::names[target->op] + " " + print_node(target->right);
			}
			
			case Tree::SimpleNode::UnaryOp:
			{
				auto target = (Tree::UnaryOpNode *)node;
				
				return Lexeme::names[target->op] +  " " + print_node(target->value);
			}
			
			case Tree::SimpleNode::String:
			{
				auto target = (Tree::StringNode *)node;

				return std::string((const char *)target->string);
			}
			
			case Tree::SimpleNode::InterpolatedPair:
			{
				auto target = (Tree::InterpolatedPairNode *)node;
				
				return std::string((const char *)target->string) + "#{" + print_node(target->group);
			}
			
			case Tree::SimpleNode::InterpolatedString:
			{
				auto target = (Tree::InterpolatedStringNode *)node;
				
				return "\"" + join(target->pairs, "}") + "}" + std::string((const char *)target->tail) + "\"";
			}
			
			case Tree::SimpleNode::Integer:
			{
				auto target = (Tree::IntegerNode *)node;

				std::stringstream out;
				out << target->value;
				return out.str();
			}
			
			case Tree::SimpleNode::Variable:
			{
				auto target = (Tree::VariableNode *)node;
				auto var = target->var;
				
				switch(var->type)
				{
					case Tree::Variable::Temporary:
					{
						std::stringstream out;
						out << "__temp" << var->index << "__";
						return out.str();
					}
					
					default:
						if(((Tree::NamedVariable *)var)->name)
							return print_symbol(((Tree::NamedVariable *)var)->name);
						else
						{
							std::stringstream out;
							out << "__temp" << var->index << "__";
							return out.str();
						}
				}
			}
			
			case Tree::SimpleNode::IVar:
			{
				auto target = (Tree::IVarNode *)node;
				
				return print_symbol(target->name);
			};
				
			case Tree::SimpleNode::Constant:
			{
				auto target = (Tree::ConstantNode *)node;
				
				return print_symbol(target->name);
			};
			
			case Tree::SimpleNode::Self:
			{
				return "self";
			}

			case Tree::SimpleNode::Nil:
			{
				return "nil";
			}

			case Tree::SimpleNode::True:
			{
				return "true";
			}

			case Tree::SimpleNode::False:
			{
				return "false";
			}
			
			case Tree::SimpleNode::Group:
			{
				auto target = (Tree::GroupNode *)node;
				
				std::string result;
				
				for(auto i = target->statements.begin(); i; i++)
					result += get_indent(indent) + print_node(*i, indent + 1) + "\n";

				return result;
			}
			
			case Tree::SimpleNode::Array:
			{
				auto target = (Tree::ArrayNode *)node;
				
				return "[" + join(target->entries, ", ") + "]";
			}
			
			case Tree::SimpleNode::Block:
			{
				auto target = (Tree::BlockNode *)node;
				return "do\n" + print_node(target->scope->group) + "end\n";
			}

			case Tree::SimpleNode::Call:
			{
				auto target = (Tree::CallNode *)node;
				
				std::string result = print_node(target->object) + "." + print_symbol(target->method);
				
				if(!target->arguments.empty())
					result += "(" + join(target->arguments, ", ") + ")";
					
				if(target->block)
					result += " " + print_node(target->block);
					
				return result;
			}

			case Tree::SimpleNode::Super:
			{
				auto target = (Tree::SuperNode *)node;
				
				std::string result = "super";
				
				if(!target->arguments.empty())
					result += "(" + join(target->arguments, ", ") + ")";
				
				if(target->block)
					result += " " + print_node(target->block);
				
				return result;
			}
			
			case Tree::SimpleNode::If:
			{
				auto target = (Tree::IfNode *)node;
				
				return (target->inverted ? "unless " : "if ") + print_node(target->left) + "\n" + print_node(target->middle) + "\nelse\n" + print_node(target->right) + "\nend\n";
			}
			
			case Tree::SimpleNode::Rescue:
			{
				auto target = (Tree::RescueNode *)node;
				
				return "rescue\n" + print_node(target->group);
			}
			
			case Tree::SimpleNode::Handler:
			{
				auto target = (Tree::HandlerNode *)node;
				
				std::string result = "begin\n" + print_node(target->code) + join(target->rescues);
				
				if(target->ensure_group)
					result += "ensure\n" + print_node(target->ensure_group);
				
				return result + "\nend\n";
			}
			
			case Tree::SimpleNode::Return:
			{
				auto target = (Tree::ReturnNode *)node;
				
				return "return " + print_node(target->value);
			}
			
			case Tree::SimpleNode::Next:
			{
				auto target = (Tree::NextNode *)node;
				
				return "return " + print_node(target->value);
			}
			
			case Tree::SimpleNode::Break:
			{
				auto target = (Tree::BreakNode *)node;
				
				return "return " + print_node(target->value);
			}
			
			case Tree::SimpleNode::Redo:
			{
				return "redo";
			}
			
			case Tree::SimpleNode::Module:
			{
				auto target = (Tree::ModuleNode *)node;
				
				return "module " + print_symbol(target->name) + "\n" + print_node(target->scope->group) + "end\n";
			}
			
			case Tree::SimpleNode::Class:
			{
				auto target = (Tree::ClassNode *)node;
				
				return "class " + print_symbol(target->name) + " < " + print_node(target->super) + "\n" + print_node(target->scope->group) + "end\n";
			}
			
			case Tree::SimpleNode::Method:
			{
				auto target = (Tree::MethodNode *)node;
				
				return "def " + print_symbol(target->name) + "\n" + print_node(target->scope->group) + "end\n";
			}
			
			default:
				return "<unknown>";
		}
	}
	
	std::string DebugPrinter::wrap(Tree::SimpleNode *node, std::string result)
	{
		if(node)
		{
			return "(" + Tree::SimpleNode::names[node->type()] + ": " +  result + ")";
		}
		else
			return result;
	}
};
