#pragma once
#include "../common.hpp"
#include <Prelude/CountedList.hpp>
#include "../generic/range.hpp"

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
				String,
				Interpolated,
				InterpolatedPair,
				Integer,
				Variable,
				IVar,
				Global,
				Constant,
				UnaryOp,
				BooleanNot,
				BinaryOp,
				BooleanOp,
				Assignment,
				Self,
				Nil,
				True,
				False,
				Symbol,
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
		
		typedef List<Node, ListNode> NodeList;
		typedef CountedList<Node, ListNode> CountedNodeList;
	};
};
