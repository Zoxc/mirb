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
				InterpolatedString,
				InterpolatedPair,
				Integer,
				Variable,
				IVar,
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
				Array,
				Block,
				Invoke,
				Call,
				Super,
				If,
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
				Types
			};
			
			static std::string names[Types];

			virtual NodeType type() { return None; };
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
