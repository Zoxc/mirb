#include "parser.hpp"

namespace Mirb
{
	Node *Parser::parse_class()
	{
		lexer.step();

		auto result = new (memory_pool) ClassNode;
		
		if(require(Lexeme::IDENT))
		{
			result->name = lexer.lexeme.symbol;
		
			lexer.step();
		}
		else
			result->name = 0;
		
		if(lexeme() == Lexeme::LESS)
		{
			lexer.step();

			result->super = parse_expression();
		}
		else
			result->super = new (memory_pool) VariableNode(*this, (Symbol *)rt_symbol_from_cstr("Object"), 0);
		
		parse_sep();
		
		struct block *block = alloc_scope(result->scope, S_CLASS);
		
		parse_statements(result->scope.statements);
		
		current_block = block->parent;
		
		match(Lexeme::KW_END);
		
		return result;
	}

	Node *Parser::parse_module()
	{
		lexer.step();

		auto result = new (memory_pool) ModuleNode;
		
		if(require(Lexeme::IDENT))
		{
			result->name = lexer.lexeme.symbol;

			lexer.step();
		}
		else
			result->name = 0;

		parse_sep();

		struct block *block = alloc_scope(result->scope, S_MODULE);
		
		parse_statements(result->scope.statements);
		
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

	void Parser::parse_parameters(struct block *block)
	{
		do
		{
			if(!is_parameter())
			{
				error("Expected paramater, but found " + lexer.lexeme.describe());
				return;
			}
			
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
		}
		while(matches(Lexeme::COMMA));
	}

	Node *Parser::parse_method()
	{
		lexer.lexeme.allow_keywords = false;
		
		lexer.step();
		
		lexer.lexeme.allow_keywords = true;
		
		auto result = new (memory_pool) MethodNode;
		
		switch(lexeme())
		{
			case Lexeme::IDENT:
			case Lexeme::EXT_IDENT:
				{
					result->name = lexer.lexeme.symbol;
					
					lexer.step();
				}
				break;

			default:
				{
					expected(Lexeme::IDENT);
					result->name = 0;
				}
		}
		
		struct block *block = alloc_scope(result->scope, S_METHOD);
		
		block->owner = block;

		if(matches(Lexeme::PARENT_OPEN))
		{
			parse_parameters(block);
			
			match(Lexeme::PARENT_CLOSE);
		}
		else
		{
			if(is_parameter())
				parse_parameters(block);
			
			parse_sep();
		}
		
		parse_statements(result->scope.statements);
		
		current_block = block->parent;
		
		match(Lexeme::KW_END);
		
		return result;
	}
};
