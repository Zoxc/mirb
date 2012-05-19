#pragma once
#include "../common.hpp"
#include <Prelude/CountedList.hpp>
#include "../generic/source-loc.hpp"

namespace Mirb
{
	namespace Tree
	{
		struct Node;
		
		struct SimpleNode
		{
			enum NodeType
			{
				None,
				Data,
				Heredoc,
				Interpolate,
				Integer,
				Float,
				Variable,
				CVar,
				IVar,
				Global,
				Constant,
				UnaryOp,
				BooleanNot,
				BinaryOp,
				Assignment,
				Self,
				Nil,
				True,
				False,
				Symbol,
				NodeRange,
				Array,
				Hash,
				Block,
				Invoke,
				Call,
				Super,
				If,
				Case,
				Loop,
				Group,
				Void,
				Return,
				Next,
				Break,
				Redo,
				Class,
				Module,
				Method,
				Alias,
				Rescue,
				Handler,
				Splat,
				MultipleExpressions,
				Types
			};
			
			static std::string names[Types];

			virtual NodeType type() { return None; };

			bool single() { return (this == nullptr) || (type() != MultipleExpressions); }
		};
		
		struct ListNode:
			public SimpleNode
		{
			ListEntry<ListNode> entry;
		};
		
		struct Node:
			public ListNode
		{
		};
		
		struct LocationNode:
			public Node
		{
			SourceLoc range;
			
			LocationNode() {}
			LocationNode(const SourceLoc &range) : range(range) {}
		};
		
		typedef List<Node, ListNode> NodeList;
		typedef CountedList<Node, ListNode> CountedNodeList;
	};
};
