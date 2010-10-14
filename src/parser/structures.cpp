#include "parser.hpp"

namespace Mirb
{
	struct node *Parser::parse_class()
	{
		lexer.step();

		struct node *result = alloc_node(N_CLASS);

		if(require(Lexeme::IDENT))
		{
			result->left = (struct node *)(rt_value)lexer.lexeme.symbol;

			lexer.step();
		}
		else
			result->left = 0;

		if(lexeme() == Lexeme::LESS)
		{
			lexer.step();

			result->middle = parse_expression();
		}
		else
			result->middle = 0;

		parse_sep();

		struct block *block;

		result->right = alloc_scope(&block, S_CLASS);
		result->right->right = parse_statements();

		current_block = block->parent;

		match(Lexeme::KW_END);

		return result;
	}

	struct node *Parser::parse_module()
	{
		lexer.step();

		struct node *result = alloc_node(N_MODULE);

		if(require(Lexeme::IDENT))
		{
			result->left = (struct node *)(rt_value)lexer.lexeme.symbol;

			lexer.step();
		}
		else
			result->left = 0;

		parse_sep();

		struct block *block;

		result->right = alloc_scope(&block, S_MODULE);
		result->right->right = parse_statements();

		current_block = block->parent;

		match(Lexeme::KW_END);

		return result;
	}

	bool Parser::is_parameter()
	{
		switch(lexeme())
		{
			case Lexeme::AMPERSAND:
			case Lexeme::IDENT:
				return true;

			default:
				return false;
		}
	}

	void Parser::parse_parameter(struct block *block)
	{
		if(is_parameter())
		{
			bool block_parameter = matches(Lexeme::AMPERSAND);

			rt_value symbol = (rt_value)lexer.lexeme.symbol;

			if(block_parameter)
			{
				if(block->block_parameter)
					error("You can only receive the block in one parameter.");
				else
					block->block_parameter = scope_define(block, symbol);
			}
			else
			{
				if(scope_defined(block, symbol, false))
					error("Variable " + lexer.lexeme.string() + " already defined.");
				else
					vec_push(variables, &block->parameters, scope_define(block, symbol));
			}

			lexer.step();

			if(matches(Lexeme::COMMA))
				parse_parameter(block);
		}
		else
			error("Expected paramater, but found " + lexer.lexeme.describe());
	}

	struct node *Parser::parse_method()
	{
		lexer.lexeme.allow_keywords = false;

		lexer.step();

		lexer.lexeme.allow_keywords = true;

		struct node *result = alloc_node(N_METHOD);

		switch(lexeme())
		{
			case T_IDENT:
			case T_EXT_IDENT:
				{
					result->left = (struct node *)(rt_value)lexer.lexeme.symbol;

					lexer.step();
				}
				break;

			default:
				{
					expected(Lexeme::IDENT);
					result->left = 0;
				}
		}

		struct block *block;

		result->right = alloc_scope(&block, S_METHOD);

		block->owner = block;

		if(matches(Lexeme::PARENT_OPEN))
		{
			parse_parameter(block);

			match(Lexeme::PARENT_CLOSE);
		}
		else
		{
			if(is_parameter())
				parse_parameter(block);

			parse_sep();
		}

		result->right->right = parse_statements();

		current_block = block->parent;

		match(Lexeme::KW_END);

		return result;
	}
};
