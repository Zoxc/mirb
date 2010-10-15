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
			Boolean,
			Array,
			Block,
			Call,
			
			If,
			Unless,
			Super,
			ZSuper,
			Return,
			Next,
			Redo,
			Break,
			BreakHandler,
			Handler,
			Rescue,
			ArrayCall,
			Class,
			Module,
			Scope,
			Method,
			Parameter
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
