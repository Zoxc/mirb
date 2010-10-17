#include "parser.hpp"

namespace Mirb
{
	Node *Parser::parse_return()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type == S_CLOSURE)
			current_block->owner->require_exceptions = true; // Make sure our parent can handle the return exception.
		
		if(is_expression())
			return new (memory_pool) ReturnNode(range, parse_expression());
		else
			return new (memory_pool) ReturnNode(range, new (memory_pool) NilNode);
	}

	Node *Parser::parse_next()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type != S_CLOSURE)
			error("Next outside of block.");
		
		if(is_expression())
			return new (memory_pool) NextNode(range, parse_expression());
		else
			return new (memory_pool) NextNode(range, new (memory_pool) NilNode);
	}

	Node *Parser::parse_redo()
	{
		auto range = new (memory_pool) Range(lexer.lexeme);
		
		lexer.step();
		
		if(current_block->type != S_CLOSURE)
			error("Redo outside of block.");
		
		return new (memory_pool) RedoNode(range);
	}

	Node *Parser::parse_break()
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
			return new (memory_pool) BreakNode(range, parse_expression());
		else
			return new (memory_pool) BreakNode(range, new (memory_pool) NilNode);
	}

	Node *Parser::parse_exception_handlers(Node *block)
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
		
		auto result = new (memory_pool) HandlerNode;
		
		result->code = block;
		
		while(lexeme() == Lexeme::KW_RESCUE);
		{
			lexer.step();
			
			auto rescue = new (memory_pool) RescueNode;
			
			parse_statements(rescue->statements);
			
			result->rescues.append(rescue);
		}
		
		if(matches(Lexeme::KW_ENSURE))
			result->group = parse_group();
		else
			result->group = 0;
		
		return result;
	}

	Node *Parser::parse_begin()
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

	Node *Parser::parse_ternary_if()
	{
		Node *result = parse_precedence_operator();
		
		if(lexeme() == Lexeme::QUESTION)
		{
			lexer.step();
			
			auto node = new (memory_pool) IfNode;
			
			node->inverted = false;
			
			node->left = result;
			node->middle = parse_ternary_if();
			
			match(Lexeme::COLON);
			
			node->right = parse_ternary_if();
			
			return node;
		}
		
		return result;
	}

	Node *Parser::parse_conditional()
	{
		Node *result = parse_boolean();
		
		if (lexeme() == Lexeme::KW_IF || lexeme() == Lexeme::KW_UNLESS)
		{
			auto node = new (memory_pool) IfNode;
			
			node->inverted = lexeme() == Lexeme::KW_UNLESS;
			
			lexer.step();
			
			node->middle = result;
			node->left = parse_statement();
			node->right = new (memory_pool) NilNode;
			
			return node;
		}
		
		return result;
	}

	Node *Parser::parse_unless()
	{
		lexer.step();

		auto result = new (memory_pool) IfNode;
		
		result->inverted = true;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = new (memory_pool) NilNode;
		
		match(Lexeme::KW_END);
		
		return result;
	}

	Node *Parser::parse_if_tail()
	{
		switch (lexeme())
		{
			case T_ELSIF:
				{
					lexer.step();
					
					auto result = new (memory_pool) IfNode;
					
					result->inverted = false;
					
					result->left = parse_expression();
					
					parse_then_sep();
					
					result->middle = parse_group();
					result->right = parse_if_tail();
					
					return result;
				}

			case T_ELSE:
				lexer.step();

				return parse_group();

			default:
				return new (memory_pool) NilNode;
		}
	}

	Node *Parser::parse_if()
	{
		lexer.step();
		
		auto result = new (memory_pool) IfNode;
		
		result->inverted = false;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = parse_if_tail();
		
		match(Lexeme::KW_END);
		
		return result;
	}

	Node *Parser::parse_case()
	{
		lexer.step();

		IfNode *result = 0;
		IfNode *first = 0;
		
		do
		{
			match(Lexeme::KW_WHEN);
			lexer.step();
			
			auto node = new (memory_pool) IfNode;
			
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
			first->right = new (memory_pool) NilNode;
		
		match(Lexeme::KW_END);

		return result;
	}
};
