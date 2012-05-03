#pragma once
#include "../common.hpp"
#include <Prelude/List.hpp>
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
			
			Range *range;
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
			UnaryOpNode(Lexeme::Type op, Node *value, Range *range) : op(op), value(value), range(range) {}
			
			Lexeme::Type op;
			Node *value;
			
			Range *range;
		};
		
		struct BooleanNotNode:
			public UnaryOpNode
		{
			NodeType type() { return BooleanNot; }
			
			BooleanNotNode(Node *value) : UnaryOpNode(Lexeme::LOGICAL_NOT, value, nullptr) {}
		};
		
		struct StringNode:
			public Node
		{
			NodeType type() { return String; }
			
			Value::Type result_type;
			
			StringData::Entry string;
		};
		
		struct InterpolatedPairNode:
			public ListNode
		{
			NodeType type() { return InterpolatedPair; }
			
			StringData::Entry string;

			Node *group;
		};
		
		typedef List<InterpolatedPairNode, ListNode> InterpolatedPairList;
		
		struct InterpolatedNode:
			public StringNode
		{
			NodeType type() { return Interpolated; }
			
			InterpolatedPairList pairs;
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
			
			Mirb::Symbol *name;
			
			IVarNode(Mirb::Symbol *name) : name(name) {}
		};
		
		struct GlobalNode:
			public Node
		{
			NodeType type() { return Global; }
			
			Mirb::Symbol *name;
			
			GlobalNode(Mirb::Symbol *name) : name(name) {}
		};
		
		struct ConstantNode:
			public Node
		{
			NodeType type() { return Constant; }

			bool top_scope;
	
			Node *obj;
			Mirb::Symbol *name;

			Range *range;
			
			ConstantNode() {}
			ConstantNode(Node *obj, Mirb::Symbol *name, Range *range, bool top_scope = false) : top_scope(top_scope), obj(obj), name(name), range(range) {}
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
		
		struct SymbolNode:
			public Node
		{
			NodeType type() { return Symbol; }
			
			Mirb::Symbol *symbol;
		};
		
		struct ArrayNode:
			public Node
		{
			NodeType type() { return Array; }
			
			CountedNodeList entries;
		};
		
		struct HashNode:
			public Node
		{
			NodeType type() { return Hash; }
			
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

			Range *range;
		};
		
		struct CallNode:
			public InvokeNode
		{
			NodeType type() { return Call; }
			
			bool can_be_var;
			bool subscript;

			Node *object;
			Mirb::Symbol *method;

			CallNode() : can_be_var(false), subscript(false) {}
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
			
			bool top_scope;
	
			Node *scoped;
			Mirb::Symbol *name;
			Scope *scope;

			Range *range;
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
			
			Node *singleton;
			Mirb::Symbol *name;
			Scope *scope;
			Range *range;
		};
	};
};
