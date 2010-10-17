#pragma once
#include "../simple-list.hpp"

namespace Mirb
{
	struct SimpleNode
	{
		enum NodeType
		{
			None,
			
			// Expressions
			String,
			InterpolatedString,
			InterpolatedPair,
			Integer,
			Variable,
			UnaryOp,
			BinaryOp,
			Assignment,
			Self,
			Nil,
			True,
			False,
			Array,
			Parameter,
			Block,
			Invoke,
			Call,
			Super,
			BreakHandler,
			If,
			Group,
			Void,
			Return,
			Next,
			Break,
			Redo,
			Class,
			Module,
			Rescue,
			Handler,
			Method
		};
		
		virtual NodeType type() { return None; };
	};
	
	struct ListNode:
		public SimpleNode
	{
		SimpleEntry<ListNode> entry;
	};
	
	struct Node:
		public ListNode
	{
	};
	
	typedef SimpleList<Node, ListNode> NodeList;
};
