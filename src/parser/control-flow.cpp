#include "parser.hpp"

namespace Mirb
{
	Tree::Node *Parser::parse_return()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type == S_CLOSURE)
			current_block->owner->require_exceptions = true; // Make sure our parent can handle the return exception.
		
		if(is_expression())
			return new (memory_pool) Tree::ReturnNode(range, parse_expression());
		else
			return new (memory_pool) Tree::ReturnNode(range, new (memory_pool) Tree::NilNode);
	}

	Tree::Node *Parser::parse_next()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type != S_CLOSURE)
			error("Next outside of block.");
		
		if(is_expression())
			return new (memory_pool) Tree::NextNode(range, parse_expression());
		else
			return new (memory_pool) Tree::NextNode(range, new (memory_pool) Tree::NilNode);
	}

	Tree::Node *Parser::parse_redo()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type != S_CLOSURE)
			error("Redo outside of block.");
		
		return new (memory_pool) Tree::RedoNode(range);
	}

	Tree::Node *Parser::parse_break()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type == S_CLOSURE)
		{
			current_block->can_break = true; // Flag our parent should check
		}
		else
			error("Break outside of block.");
		
		if(is_expression())
			return new (memory_pool) Tree::BreakNode(range, parse_expression());
		else
			return new (memory_pool) Tree::BreakNode(range, new (memory_pool) Tree::NilNode);
	}

	Tree::Node *Parser::parse_exception_handlers(Tree::Node *block)
	{
		switch (lexeme())
		{
			case Lexeme::KW_ENSURE:
			case Lexeme::KW_RESCUE:
				break;
			
			default:
				return block;
				break;
		}
		
		auto result = new (memory_pool) Tree::HandlerNode;
		
		result->code = block;
		
		while(lexeme() == Lexeme::KW_RESCUE)
		{
			lexer.step();
			
			auto rescue = new (memory_pool) Tree::RescueNode;
			
			rescue->group = parse_group();
			
			result->rescues.append(rescue);
		}
		
		if(matches(Lexeme::KW_ENSURE))
			result->ensure_group = parse_group();
		else
			result->ensure_group = 0;
		
		return result;
	}

	Tree::Node *Parser::parse_begin()
	{
		lexer.step();
		
		auto result = parse_exception_handlers(parse_group());
		
		match(Lexeme::KW_END);
		
		return result;
	}

	void Parser::parse_then_sep()
	{
		switch (lexeme())
		{
			case Lexeme::KW_THEN:
			case Lexeme::COLON:
				lexer.step();
				break;

			default:
				parse_sep();
		}
	}

	Tree::Node *Parser::parse_ternary_if()
	{
		Tree::Node *result = parse_precedence_operator();
		
		if(lexeme() == Lexeme::QUESTION)
		{
			lexer.step();
			
			auto node = new (memory_pool) Tree::IfNode;
			
			node->inverted = false;
			
			node->left = result;
			node->middle = parse_ternary_if();
			
			match(Lexeme::COLON);
			
			node->right = parse_ternary_if();
			
			return node;
		}
		
		return result;
	}

	Tree::Node *Parser::parse_conditional()
	{
		Tree::Node *result = parse_boolean();
		
		if (lexeme() == Lexeme::KW_IF || lexeme() == Lexeme::KW_UNLESS)
		{
			auto node = new (memory_pool) Tree::IfNode;
			
			node->inverted = lexeme() == Lexeme::KW_UNLESS;
			
			lexer.step();
			
			node->middle = result;
			node->left = parse_statement();
			node->right = new (memory_pool) Tree::NilNode;
			
			return node;
		}
		
		return result;
	}

	Tree::Node *Parser::parse_unless()
	{
		lexer.step();

		auto result = new (memory_pool) Tree::IfNode;
		
		result->inverted = true;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = new (memory_pool) Tree::NilNode;
		
		match(Lexeme::KW_END);
		
		return result;
	}

	Tree::Node *Parser::parse_if_tail()
	{
		switch (lexeme())
		{
			case Lexeme::KW_ELSIF:
				{
					lexer.step();
					
					auto result = new (memory_pool) Tree::IfNode;
					
					result->inverted = false;
					
					result->left = parse_expression();
					
					parse_then_sep();
					
					result->middle = parse_group();
					result->right = parse_if_tail();
					
					return result;
				}

			case Lexeme::KW_ELSE:
				lexer.step();

				return parse_group();

			default:
				return new (memory_pool) Tree::NilNode;
		}
	}

	Tree::Node *Parser::parse_if()
	{
		lexer.step();
		
		auto result = new (memory_pool) Tree::IfNode;
		
		result->inverted = false;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = parse_if_tail();
		
		match(Lexeme::KW_END);
		
		return result;
	}

	Tree::Node *Parser::parse_case()
	{
		lexer.step();

		Tree::IfNode *result = 0;
		Tree::IfNode *first = 0;
		
		do
		{
			match(Lexeme::KW_WHEN);
			lexer.step();
			
			auto node = new (memory_pool) Tree::IfNode;
			
			first = first ? first : node;
			
			node->inverted = false;
			node->left = parse_expression();
			
			parse_then_sep();
			
			node->middle = parse_group();
			node->right = result;
			
			result = node;
		}
		while(lexeme() == Lexeme::KW_WHEN);
		
		if(matches(Lexeme::KW_ELSE))
		{
			first->right = parse_group();
		}
		else
			first->right = new (memory_pool) Tree::NilNode;
		
		match(Lexeme::KW_END);

		return result;
	}
};
