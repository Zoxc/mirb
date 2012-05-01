#include "node.hpp"

namespace Mirb
{
	std::string Tree::SimpleNode::names[] = {
		"None",
		"String",
		"Interpolated",
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
		"Symbol",
		"Array",
		"Hash",
		"Block",
		"Invoke",
		"Call",
		"Super",
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
