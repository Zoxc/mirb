#include "parser.hpp"

namespace Mirb
{
	struct node *Parser::parse_return()
	{
		lexer.step();

		struct node *result = alloc_node(N_RETURN);

		if(current_block->type == S_CLOSURE)
			current_block->owner->require_exceptions = true; // Make sure our parent can handle the return exception.

		if(is_expression())
			result->left = parse_expression();
		else
			result->left = &nil_node;

		return result;
	}

	struct node *Parser::parse_next()
	{
		lexer.step();

		struct node *result = alloc_node(N_NEXT);

		if(current_block->type != S_CLOSURE)
			error("Next outside of block.");

		if(is_expression())
			result->left = parse_expression();
		else
			result->left = &nil_node;

		return result;
	}

	struct node *Parser::parse_redo()
	{
		lexer.step();

		struct node *result = alloc_node(N_REDO);

		if(current_block->type != S_CLOSURE)
			error("Redo outside of block.");

		return result;
	}

	struct node *Parser::parse_break()
	{
		lexer.step();

		struct node *result = alloc_node(N_BREAK);

		if(current_block->type == S_CLOSURE)
		{
			current_block->can_break = true; // Flag our parent should check
		}
		else
			error("Break outside of block.");

		if(is_expression())
			result->left = parse_expression();
		else
			result->left = &nil_node;

		return result;
	}

	struct node *Parser::parse_exception_handlers(struct node *block)
	{
		struct node *parent = alloc_node(N_HANDLER);

		parent->left = block;

		if(lexeme() == Lexeme::KW_RESCUE)
		{
			lexer.step();

			struct node *rescue = alloc_node(N_RESCUE);
			rescue->left = parse_statements();
			rescue->right = 0;

			parent->middle = rescue;

			while(lexeme() == Lexeme::KW_RESCUE)
			{
				lexer.step();

				struct node *node = alloc_node(N_RESCUE);
				node->left = parse_statements();
				node->right = 0;

				rescue->right = node;
				rescue = node;
			}
		}
		else
			parent->middle = 0;

		if(lexeme() == Lexeme::KW_ENSURE)
		{
			lexer.step();

			parent->right = parse_statements();
		}
		else
			parent->right = 0;

		return parent;
	}

	struct node *Parser::parse_begin()
	{
		lexer.step();

		struct node *result = parse_statements();

		switch (lexeme())
		{
			case T_ENSURE:
			case T_RESCUE:
				result = parse_exception_handlers(result);
				break;

			default:
				break;
		}

		match(Lexeme::KW_END);

		return result;
	}

	void Parser::parse_then_sep()
	{
		switch (lexeme())
		{
			case T_THEN:
			case T_COLON:
				lexer.step();
				break;

			default:
				parse_sep();
		}
	}

	struct node *Parser::parse_ternary_if()
	{
		struct node *result = parse_boolean_or();

		if(lexeme() == Lexeme::QUESTION)
		{
			lexer.step();

			struct node *node = alloc_node(N_IF);

			node->left = result;
			node->middle = parse_ternary_if();

			match(Lexeme::COLON);

			node->right = parse_ternary_if();

			return node;
		}

		return result;
	}

	struct node *Parser::parse_conditional()
	{
		struct node *result = parse_low_boolean();

		if (lexeme() == Lexeme::KW_IF || lexeme() == Lexeme::KW_UNLESS)
		{
			struct node *node = alloc_node(lexeme() == Lexeme::KW_IF ? N_IF : N_UNLESS);

			lexer.step();

			node->middle = result;
			node->left = parse_statement();
			node->right = &nil_node;

			return node;
		}

		return result;
	}

	struct node *Parser::parse_unless()
	{
		lexer.step();

		struct node *result = alloc_node(N_UNLESS);
		result->left = parse_expression();

		parse_then_sep();

		result->middle = parse_statements();
		result->right = &nil_node;

		match(Lexeme::KW_END);

		return result;
	}

	struct node *Parser::parse_if_tail()
	{
		switch (lexeme())
		{
			case T_ELSIF:
				{
					lexer.step();

					struct node *result = alloc_node(N_IF);
					result->left = parse_expression();

					parse_then_sep();

					result->middle = parse_statements();
					result->right = parse_if_tail();

					return result;
				}

			case T_ELSE:
				lexer.step();

				return parse_statements();

			default:
				return &nil_node;
		}
	}

	struct node *Parser::parse_if()
	{
		lexer.step();

		struct node *result = alloc_node(N_IF);
		result->left = parse_expression();

		parse_then_sep();

		result->middle = parse_statements();
		result->right = parse_if_tail();

		match(Lexeme::KW_END);

		return result;
	}

	struct node *Parser::parse_case_body()
	{
		switch (lexeme())
		{
			case T_WHEN:
				{
					lexer.step();

					struct node *result = alloc_node(N_IF);
					result->left = parse_expression();

					parse_then_sep();

					result->middle = parse_statements();
					result->right = parse_case_body();

					return result;
				}

			case T_ELSE:
				{
					lexer.step();

					return parse_statements();
				}

			default:
				{
					error("Expected else or when but found " + lexer.lexeme.describe());

					return 0;
				}
		}
	}

	struct node *Parser::parse_case()
	{
		lexer.step();

		struct node *result = 0;

		switch (lexeme())
		{
			case T_ELSE:
				lexer.step();

				result = parse_statements();
				break;

			case T_WHEN:
				result = parse_case_body();
				break;

			default:
				error("Expected else or when but found " + lexer.lexeme.describe());
		}

		match(Lexeme::KW_END);

		return result;
	}
};
