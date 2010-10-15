#pragma once
#include "../common.hpp"
#include "../symbol-pool.hpp"
#include "node.hpp"

namespace Mirb
{
	struct BinaryOpNode:
		public Node
	{
		NodeType type() { return BinaryOp; }
		
		Node *left;
		Node *right;
		Lexeme::Type op;
	};

	struct AssignmentNode:
		public BinaryOpNode
	{
		NodeType type() { return Assignment; }
	};
	
	struct UnaryOpNode:
		public Node
	{
		NodeType type() { return UnaryOp; }
		
		Lexeme::Type op;
		Node *value;
	};

	struct StringNode:
		public Node
	{
		NodeType type() { return String; }
		
		const char_t *string;
	};
	
	struct InterpolatedPairNode:
		public ListNode
	{
		NodeType type() { return InterpolatedPair; }
		
		const char_t *string;
		Node *expression;
	};
	
	typedef SimpleList<InterpolatedPairNode, ListNode>  InterpolatedPairNodeList;
	
	struct InterpolatedStringNode:
		public Node
	{
		NodeType type() { return InterpolatedString; }
		
		InterpolatedPairNodeList pairs;
		
		const char_t *tail;
	};
	
	struct IntegerNode:
		public Node
	{
		NodeType type() { return Integer; }
		
		int value;
	};

	struct VariableNode:
		public Node
	{
		NodeType type() { return Variable; }
		
		enum VariableType
		{
			Local,
			Instance,
			Constant,
			Global
		};
		
		VariableType variable_type;
		Symbol *name;
	};

	struct SelfNode:
		public Node
	{
		NodeType type() { return Self; }
	};

	struct NilNode:
		public Node
	{
		NodeType type() { return Nil; }
	};

	struct BooleanNode:
		public Node
	{
		NodeType type() { return Boolean; }
		
		bool value;
	};
	
	struct ArrayNode:
		public Node
	{
		NodeType type() { return Array; }
		
		NodeList entries;
	};
	
	struct BlockNode:
		public SimpleNode
	{
		NodeType type() { return Block; }
		
		NodeList statements;
	};
	
	struct CallNode:
		public Node
	{
		NodeType type() { return Call; }
		
		Node *object;
		Symbol *method;
		NodeList arguments;
		BlockNode *block;
	};
};
