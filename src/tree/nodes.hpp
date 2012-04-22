#pragma once
#include <Prelude/List.hpp>
#include "../common.hpp"
#include "../lexer/lexeme.hpp"
#include "../symbol-pool.hpp"
#include "node.hpp"

namespace Mirb
{
	class Parser;
	
	namespace Tree
	{
		class Scope;
		class Variable;
		
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
		
		struct BooleanOpNode:
			public BinaryOpNode
		{
			NodeType type() { return BooleanOp; }
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
		
		struct BooleanNotNode:
			public UnaryOpNode
		{
			NodeType type() { return BooleanNot; }
			
			BooleanNotNode(Node *value) : UnaryOpNode(Lexeme::LOGICAL_NOT, value) {}
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
		
		typedef List<InterpolatedPairNode, ListNode> InterpolatedPairList;
		
		struct InterpolatedStringNode:
			public Node
		{
			NodeType type() { return InterpolatedString; }
			
			InterpolatedPairList pairs;
			
			const char_t *tail; // TODO: Replace with a nicer type
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
			
			Tree::Variable *var;
			
			VariableNode() {}
			VariableNode(Tree::Variable *var) : var(var) {}
		};
		
		struct IVarNode:
			public Node
		{
			NodeType type() { return IVar; }
			
			Symbol *name;
			
			IVarNode(Symbol *name) : name(name) {}
		};
		
		struct ConstantNode:
			public Node
		{
			NodeType type() { return Constant; }
	
			Node *obj;
			Symbol *name;
			
			ConstantNode() {}
			ConstantNode(Symbol *name) : obj(0), name(name)  {}
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
			
			CountedNodeList entries;
		};
		
		struct BlockNode:
			public SimpleNode
		{
			NodeType type() { return Block; }
			
			Scope *scope;
		};
		
		struct InvokeNode:
			public Node
		{
			NodeType type() { return Invoke; }
			
			CountedNodeList arguments;
			BlockNode *block; // can be zero
		};
		
		struct CallNode:
			public InvokeNode
		{
			NodeType type() { return Call; }
			
			Node *object;
			Symbol *method;
			bool can_be_var;
		};
		
		struct SuperNode:
			public InvokeNode
		{
			NodeType type() { return Super; }
			
			bool pass_args;
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
		
		typedef List<RescueNode, ListNode>  RescueList;
		
		struct HandlerNode:
			public Node
		{
			NodeType type() { return Handler; }
			
			Node *code;
			RescueList rescues;
			Node *ensure_group;
		};
		
		//TODO: Make sure no void nodes are in found in expressions.
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
			Scope *scope;
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
			Scope *scope;
		};
	};
};
