#pragma once
#include "../simple-list.hpp"

namespace Mirb
{
	struct Node
	{
		enum NodeType
		{
			None,

			UnaryOp,
			BinaryOp,
			Number,
			Var,
			IVar,
			IVarAssign,
			String,
			StringStart,
			StringContinue,
			
			Array,
			ArrayElement,
			Const,
			Self,
			True,
			False,
			Nil,
			Assignment,
			ConstAssignment,
			Boolean,
			Not,
			NotEqual,
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
			Argument,
			CallArguments,
			Call,
			ArrayCall,
			Statements,
			Class,
			Module,
			Scope,
			Method,
			Parameter
		};
		
		static const NodeType type = None;
	};
	
	struct ListNode:
		public Node
	{
		SimpleEntry<ListNode> entry;
	};
};
