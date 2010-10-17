#include "../parser/parser.hpp"
#include "nodes.hpp"

namespace Mirb
{
	VariableNode::VariableNode()
	{
	}
	
	VariableNode::VariableNode(VariableType type, Symbol *symbol) : variable_type(type)
	{
		ivar.name = symbol;
	}
	
	VariableNode::VariableNode(Parser &parser, Symbol *symbol, Node *left)
	{
		if(rt_symbol_is_const((rt_value)symbol))
		{
			variable_type = VariableNode::Constant;
			
			constant.left = left ? left : new (parser.memory_pool) SelfNode;
			constant.name = symbol;
		}
		else
		{
			variable_type = VariableNode::Local;
			
			var = parser.scope_get(parser.current_block, (rt_value)symbol);
		}
	}
};
