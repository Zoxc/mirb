#include "parser.hpp"

namespace Mirb
{
	Tree::Node *Parser::parse_return()
	{
		auto range = capture();
		
		lexer.step();
		
		if(scope->type == Tree::Scope::Closure)
			scope->owner->require_exceptions = true; // Make sure our parent can handle the return exception.
		
		if(is_expression())
			return new (fragment) Tree::ReturnNode(range, parse_expression());
		else
			return new (fragment) Tree::ReturnNode(range, new (fragment) Tree::NilNode);
	}

	Tree::Node *Parser::parse_next()
	{
		auto range = capture();
		
		if(scope->type != Tree::Scope::Closure)
			error("Next outside of block.");
		
		lexer.step();
		
		if(is_expression())
			return new (fragment) Tree::NextNode(range, parse_expression());
		else
			return new (fragment) Tree::NextNode(range, new (fragment) Tree::NilNode);
	}

	Tree::Node *Parser::parse_redo()
	{
		auto range = capture();
		
		if(scope->type != Tree::Scope::Closure)
			error("Redo outside of block.");
		
		lexer.step();
		
		return new (fragment) Tree::RedoNode(range);
	}

	Tree::Node *Parser::parse_break()
	{
		auto range = capture();
		
		if(scope->type == Tree::Scope::Closure)
		{
			if(scope->break_id == Tree::Scope::no_break_id)
				scope->break_id = scope->parent->break_targets++;
		}
		else
			error("Break outside of block.");
		
		lexer.step();
		
		if(is_expression())
			return new (fragment) Tree::BreakNode(range, parse_expression());
		else
			return new (fragment) Tree::BreakNode(range, new (fragment) Tree::NilNode);
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
		
		auto result = new (fragment) Tree::HandlerNode;
		
		result->code = block;
		
		while(lexeme() == Lexeme::KW_RESCUE)
		{
			lexer.step();
			
			auto rescue = new (fragment) Tree::RescueNode;
			
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
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				lexer.step();
			
				auto node = new (fragment) Tree::IfNode;
			
				node->inverted = false;
			
				node->left = result;
				node->middle = parse_expression();
			
				match(Lexeme::COLON);
			
				node->right = parse_expression();

				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_conditional()
	{
		Tree::Node *result = parse_boolean();
		
		if (lexeme() == Lexeme::KW_IF || lexeme() == Lexeme::KW_UNLESS)
		{
			auto node = new (fragment) Tree::IfNode;
			
			node->inverted = lexeme() == Lexeme::KW_UNLESS;
			
			lexer.step();
			
			node->middle = result;
			node->left = parse_statement();
			node->right = new (fragment) Tree::NilNode;
			
			return node;
		}
		
		return result;
	}

	Tree::Node *Parser::parse_unless()
	{
		lexer.step();

		auto result = new (fragment) Tree::IfNode;
		
		result->inverted = true;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = new (fragment) Tree::NilNode;
		
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
					
					auto result = new (fragment) Tree::IfNode;
					
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
				return new (fragment) Tree::NilNode;
		}
	}

	Tree::Node *Parser::parse_if()
	{
		lexer.step();
		
		auto result = new (fragment) Tree::IfNode;
		
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
			
			auto node = new (fragment) Tree::IfNode;
			
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
			first->right = new (fragment) Tree::NilNode;
		
		match(Lexeme::KW_END);

		return result;
	}
};
