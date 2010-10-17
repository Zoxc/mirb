#pragma once
#include "../common.hpp"
#include "../lexer/lexeme.hpp"
#include "../symbol-pool.hpp"
#include "node.hpp"

namespace Mirb
{
	class Parser;
	
	struct GroupNode:
		public Node
	{
		NodeType type() { return Group; }
		
		NodeList statements;
	};
	
	struct BinaryOpNode:
		public Node
	{
		NodeType type() { return BinaryOp; }
		
		Node *left;
		Node *right;
		Lexeme::Type op;
	};

	struct VariableNode;
	
	struct AssignmentNode:
		public BinaryOpNode
	{
		NodeType type() { return Assignment; }
	};
	
	struct UnaryOpNode:
		public Node
	{
		NodeType type() { return UnaryOp; }
		
		UnaryOpNode() {};
		UnaryOpNode(Lexeme::Type op, Node *value) : op(op), value(value) {}
		
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
		Node *group;
	};
	
	typedef SimpleList<InterpolatedPairNode, ListNode>  InterpolatedPairList;
	
	struct InterpolatedStringNode:
		public Node
	{
		NodeType type() { return InterpolatedString; }
		
		InterpolatedPairList pairs;
		
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
			Temporary,
			Local,
			Instance,
			Constant,
			Global
		};
		
		VariableNode();
		VariableNode(VariableType type, Symbol *symbol);
		VariableNode(Parser &parser, Symbol *symbol, Node *left = 0);
		
		VariableType variable_type;
		
		union
		{
			struct
			{
				Symbol *name;
			} ivar;
			struct
			{
				Node *left;
				Symbol *name;
			} constant;
			struct variable *var;			
		};
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

	struct TrueNode:
		public Node
	{
		NodeType type() { return True; }
	};
	
	struct FalseNode:
		public Node
	{
		NodeType type() { return False; }
	};
	
	struct ArrayNode:
		public Node
	{
		NodeType type() { return Array; }
		
		NodeList entries;
	};
	
	struct Scope
	{
		struct block *block;
		Node *group;
	};
	
	struct BlockNode:
		public SimpleNode
	{
		NodeType type() { return Block; }
		
		struct Scope scope; //TODO: wtf?
		//TODO: unused ParameterList parameters; 
	};
	
	struct InvokeNode:
		public Node
	{
		NodeType type() { return Invoke; }
		
		NodeList arguments;
		BlockNode *block; // can be zero
	};
	
	struct CallNode:
		public InvokeNode
	{
		NodeType type() { return Call; }
		
		Node *object;
		Symbol *method;
	};
	
	struct SuperNode:
		public InvokeNode
	{
		NodeType type() { return Super; }
		
		bool pass_args;
	};
	
	struct BreakHandlerNode:
		public Node
	{
		NodeType type() { return BreakHandler; }
		
		Node *code;
		struct block *block;
	};
	
	struct IfNode:
		public Node
	{
		NodeType type() { return If; }
		
		bool inverted;
		
		Node *left;
		Node *middle;
		Node *right;
	};
	
	struct RescueNode:
		public ListNode
	{
		NodeType type() { return Rescue; }
		
		Node *group;
	};
	
	typedef SimpleList<RescueNode, ListNode>  RescueList;
	
	struct HandlerNode:
		public Node
	{
		NodeType type() { return Handler; }
		
		Node *code;
		RescueList rescues;
		Node *ensure_group;
	};
	
	struct VoidNode:
		public Node
	{
		NodeType type() { return Void; }
		
		Range *range;
		
		VoidNode(Range *range) : range(range) {}
	};
	
	struct ReturnNode:
		public VoidNode
	{
		NodeType type() { return Return; }
		
		ReturnNode(Range *range, Node *value) : VoidNode(range), value(value) {}
		
		Node *value;
	};
	
	struct NextNode:
		public VoidNode
	{
		NodeType type() { return Next; }
		
		NextNode(Range *range, Node *value) : VoidNode(range), value(value) {}
		
		Node *value;
	};
	
	struct BreakNode:
		public VoidNode
	{
		NodeType type() { return Break; }
		
		BreakNode(Range *range, Node *value) : VoidNode(range), value(value) {}
		
		Node *value;
	};
	
	struct RedoNode:
		public VoidNode
	{
		NodeType type() { return Redo; }
		
		RedoNode(Range *range) : VoidNode(range) {}
	};
	
	struct ModuleNode:
		public Node
	{
		NodeType type() { return Module; }
		
		Symbol *name;
		struct Scope scope;
	};
	
	struct ClassNode:
		public ModuleNode
	{
		NodeType type() { return Class; }
		
		Node *super;
	};
	
	struct MethodNode:
		public Node
	{
		NodeType type() { return Method; }
		
		Symbol *name;
		struct Scope scope;
	};
	
	
};
