#include "node.hpp"

namespace Mirb
{
	std::string Tree::SimpleNode::names[] = {
		"None",
		"String",
		"InterpolatedString",
		"InterpolatedPair",
		"Integer",
		"Variable",
		"IVar",
		"Constant",
		"UnaryOp",
		"BooleanNot",
		"BinaryOp",
		"BooleanOp",
		"Assignment",
		"Self",
		"Nil",
		"True",
		"False",
		"Array",
		"Block",
		"Invoke",
		"Call",
		"Super",
		"BreakHandler",
		"If",
		"Group",
		"Void",
		"Return",
		"Next",
		"Break",
		"Redo",
		"Class",
		"Module",
		"Method",
		"Rescue",
		"Handler"
	};
};
