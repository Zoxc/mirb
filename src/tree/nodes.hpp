#pragma once
#include "../common.hpp"
#include <Prelude/List.hpp>
#include "../lexer/lexeme.hpp"
#include "../symbol-pool.hpp"
#include "node.hpp"

namespace Mirb
{
	class Parser;

	namespace CodeGen
	{
		class Label;
	};
	
	namespace Tree
	{
		class Scope;
		class Variable;
		class NamedVariable;

		struct LoopNode;
		
		//TODO: Make sure no void nodes are in found in expressions.
		struct VoidNode:
			public Node
		{
			NodeType type() { return Void; }
			
			SourceLoc *range;
			bool in_ensure;
			LoopNode *target;
			
			ListEntry<VoidNode> void_entry;

			VoidNode(SourceLoc *range) : range(range), in_ensure(false), target(nullptr) {}
		};
		
		typedef List<VoidNode, VoidNode, &VoidNode::void_entry> VoidList;

		class VoidTrapper
		{
			private:
			public:
				VoidTrapper *prev;
				bool trap_return;
				bool in_ensure;
				LoopNode *target;
				VoidList list;
				ListEntry<VoidTrapper> entry;

				VoidTrapper(VoidTrapper *prev, bool trap_return) : prev(prev), trap_return(trap_return), in_ensure(false), target(nullptr)
				{
				}

				void trap(VoidNode *node)
				{
					if(!trap_return && node->type() == Node::Return)
						prev->trap(node);
					else
						list.append(node);
				}
		};

		struct GroupNode:
			public Node
		{
			NodeType type() { return Group; }
			
			NodeList statements;
		};
		
		struct SplatNode:
			public Node
		{
			NodeType type() { return Splat; }
			
			Node *expression;

			SplatNode() {}
			SplatNode(Node *expression) : expression(expression) {}
		};
		
		struct MultipleExpressionNode
		{
			ListEntry<MultipleExpressionNode> entry;
			Node *expression;
			SourceLoc range;
			intptr_t index;
			size_t size;

			MultipleExpressionNode(Node *expression, const SourceLoc &range) : expression(expression), range(range) {}
		};
		
		struct MultipleExpressionsNode:
			public Node
		{
			NodeType type() { return MultipleExpressions; }
			
			SourceLoc *range;
			List<MultipleExpressionNode> expressions;
			SourceLoc *extended;
			intptr_t splat_index;
			size_t expression_count;

			MultipleExpressionsNode() : extended(nullptr) {}
		};
		
		struct BinaryOpNode:
			public LocationNode
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
			UnaryOpNode(Lexeme::Type op, Node *value, SourceLoc *range) : op(op), value(value), range(range) {}
			
			Lexeme::Type op;
			Node *value;
			
			SourceLoc *range;
		};
		
		struct BooleanNotNode:
			public UnaryOpNode
		{
			NodeType type() { return BooleanNot; }
			
			BooleanNotNode(Node *value) : UnaryOpNode(Lexeme::LOGICAL_NOT, value, nullptr) {}
		};
		
		struct DataNode:
			public LocationNode
		{
			NodeType type() { return Data; }
			
			Value::Type result_type;
			
			DataEntry data;
		};
		
		struct HeredocNode:
			public Node
		{
			NodeType type() { return Heredoc; }
			
			Node *data;

			HeredocNode() : data(nullptr) {}
		};
		
		struct InterpolatePairNode
		{
			ListEntry<InterpolatePairNode> entry;
			DataEntry string;

			Node *group;
		};
		
		struct InterpolateNode:
			public DataNode
		{
			NodeType type() { return Interpolate; }
			
			List<InterpolatePairNode> pairs;
		};
		
		struct IntegerNode:
			public Node
		{
			NodeType type() { return Integer; }
			
			void *value;
			size_t length;
		};

		struct FloatNode:
			public Node
		{
			NodeType type() { return Float; }
			
			double value;
		};

		struct VariableNode:
			public Node
		{
			NodeType type() { return Variable; }
			
			Tree::NamedVariable *var; // Might contain an unnamed variable if it's the implict block parameter or a temporary variable.
			
			VariableNode() {}
			VariableNode(Tree::NamedVariable *var) : var(var) {}
		};
		
		struct CVarNode:
			public Node
		{
			NodeType type() { return CVar; }
			
			Mirb::Symbol *name;
			
			CVarNode(Mirb::Symbol *name) : name(name) {}
		};
		
		struct IVarNode:
			public Node
		{
			NodeType type() { return IVar; }
			
			Mirb::Symbol *name;
			
			IVarNode(Mirb::Symbol *name) : name(name) {}
		};
		
		struct GlobalNode:
			public LocationNode
		{
			NodeType type() { return Global; }
			
			Mirb::Symbol *name;
		};
		
		struct ConstantNode:
			public Node
		{
			NodeType type() { return Constant; }

			bool top_scope;
	
			Node *obj;
			Mirb::Symbol *name;

			SourceLoc *range;
			
			ConstantNode() {}
			ConstantNode(Node *obj, Mirb::Symbol *name, SourceLoc *range, bool top_scope = false) : top_scope(top_scope), obj(obj), name(name), range(range) {}
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
		
		struct RangeNode:
			public LocationNode
		{
			NodeType type() { return NodeRange; }

			bool exclusive;
			Node *left;
			Node *right;
		};
		
		struct ArrayNode:
			public Node
		{
			NodeType type() { return Array; }
			
			CountedNodeList entries;
			bool variadic;
		};
		
		struct HashNode:
			public LocationNode
		{
			NodeType type() { return Hash; }
			
			CountedNodeList entries;
		};
		
		struct BlockNode:
			public Node
		{
			NodeType type() { return Block; }
			
			Scope *scope;
		};
		
		struct BlockArg
		{
			SourceLoc range;
			Node *node;

			BlockArg(const SourceLoc &range, Node *node) : range(range), node(node) {}
		};
		
		struct InvokeNode:
			public Node
		{
			NodeType type() { return Invoke; }
			
			bool variadic;
			CountedNodeList arguments;
			Scope *scope; // can be zero
			BlockArg *block_arg;

			SourceLoc *range;

			InvokeNode() : variadic(false), scope(nullptr), block_arg(nullptr) {}
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
		
		struct CaseEntry
		{
			ListEntry<CaseEntry> entry;
			SourceLoc range;
			Node *pattern;
			Node *group;
		};
		
		struct CaseNode:
			public Node
		{
			NodeType type() { return Case; }
			
			Node *value;

			List<CaseEntry> clauses;
			Node *else_clause;
		};

		struct HandlerNode;
		
		struct LoopNode:
			public Node
		{
			NodeType type() { return Loop; }
			
			bool inverted;
			
			Node *body;
			Node *condition;
			bool trap_exceptions;
			CodeGen::Label *label_start;
			CodeGen::Label *label_body;
			CodeGen::Label *label_end;

			LoopNode() : trap_exceptions(false) {}
		};
		
		struct RescueNode:
			public ListNode
		{
			NodeType type() { return Rescue; }
			
			Node *pattern;
			Node *var;
			Node *group;
		};
		
		typedef List<RescueNode, ListNode> RescueList;
		
		struct HandlerNode:
			public Node
		{
			NodeType type() { return Handler; }
			
			Node *code;
			RescueList rescues;
			Node *else_group;
			Node *ensure_group;
			LoopNode *loop;
			VoidTrapper *trapper;

			HandlerNode() : else_group(nullptr), ensure_group(nullptr), loop(nullptr) {}
		};
		
		struct ReturnNode:
			public VoidNode
		{
			NodeType type() { return Return; }
			
			ReturnNode(SourceLoc *range, Node *value) : VoidNode(range), value(value) {}
			
			Node *value;
		};
		
		struct NextNode:
			public VoidNode
		{
			NodeType type() { return Next; }
			
			NextNode(SourceLoc *range, Node *value) : VoidNode(range), value(value) {}
			
			Node *value;
		};
		
		struct BreakNode:
			public VoidNode
		{
			NodeType type() { return Break; }
			
			BreakNode(SourceLoc *range, Node *value) : VoidNode(range), value(value) {}
			
			Node *value;
		};
		
		struct RedoNode:
			public VoidNode
		{
			NodeType type() { return Redo; }
			
			RedoNode(SourceLoc *range) : VoidNode(range) {}
		};
		
		struct ModuleNode:
			public Node
		{
			NodeType type() { return Module; }
			
			bool top_scope;
	
			Node *scoped;
			Mirb::Symbol *name;
			Scope *scope;

			SourceLoc *range;

			ModuleNode() : scoped(nullptr) {}
		};
		
		struct ClassNode:
			public ModuleNode
		{
			NodeType type() { return Class; }
			
			Node *singleton;
			Node *super;
		};
		
		struct MethodNode:
			public Node
		{
			NodeType type() { return Method; }
			
			Node *singleton;
			Mirb::Symbol *name;
			Scope *scope;
			SourceLoc *range;
		};
		
		struct AliasNode:
			public LocationNode
		{
			NodeType type() { return Alias; }
			
			Node *new_name;
			Node *old_name;
		};
		
		struct UndefEntry
		{
			Node *name;
			SourceLoc range;
			ListEntry<UndefEntry> entry;
		};

		struct UndefNode:
			public Node
		{
			NodeType type() { return Undef; }
			
			List<UndefEntry> entries;
		};
	};
};
