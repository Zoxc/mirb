#include "parser.hpp"

namespace Mirb
{
	Tree::Node *Parser::parse_class()
	{
		auto range = capture();

		lexer.step();

		auto result = new (fragment) Tree::ClassNode;
		
		result->range = range;

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
			result->super = 0;
		
		lexer.lexeme.prev_set(range);

		parse_sep();
		
		
		auto old_scope = scope;
		
		result->scope = allocate_scope(Tree::Scope::Class);		
		result->scope->group = parse_group();
		
		scope = old_scope;
		
		match(Lexeme::KW_END);
		
		return result;
	}

	Tree::Node *Parser::parse_module()
	{
		auto range = capture();

		lexer.step();

		auto result = new (fragment) Tree::ModuleNode;

		result->range = range;
		
		if(require(Lexeme::IDENT))
		{
			result->name = lexer.lexeme.symbol;

			lexer.step();
		}
		else
			result->name = 0;

		lexer.lexeme.prev_set(range);

		parse_sep();

		auto old_scope = scope;
		
		result->scope = allocate_scope(Tree::Scope::Module);		
		result->scope->group = parse_group();
		
		scope = old_scope;
		
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

	void Parser::parse_parameters()
	{
		do
		{
			if(!is_parameter())
			{
				error("Expected paramater, but found " + lexer.lexeme.describe());
				return;
			}
			
			bool block_parameter = matches(Lexeme::AMPERSAND);
			
			Symbol *symbol = lexer.lexeme.symbol;
			
			if(block_parameter)
			{
				if(scope->block_parameter)
					error("You can only receive the block in one parameter.");
				else
					scope->block_parameter = scope->define<Tree::Parameter>(symbol);
			}
			else
			{
				if(scope->defined(symbol, false))
					error("Variable " + lexer.lexeme.string() + " already defined.");
				else
					scope->parameters.append(scope->define<Tree::Parameter>(symbol));
			}
			
			lexer.step();
		}
		while(matches(Lexeme::COMMA));
	}

	Tree::Node *Parser::parse_method()
	{
		lexer.lexeme.allow_keywords = false;
		
		lexer.step();
		
		lexer.lexeme.allow_keywords = true;
		
		auto result = new (fragment) Tree::MethodNode;

		Symbol *symbol;
		
		parse_method_name(symbol);

		result->name = symbol;
		
		auto old_scope = scope;
		
		result->scope = allocate_scope(Tree::Scope::Method);		
		
		scope->owner = scope;
		
		if(matches(Lexeme::PARENT_OPEN))
		{
			parse_parameters();
			
			match(Lexeme::PARENT_CLOSE);
		}
		else
		{
			if(is_parameter())
				parse_parameters();
			
			parse_sep();
		}
		
		result->scope->group = parse_group();
		
		scope = old_scope;
		
		match(Lexeme::KW_END);
		
		return result;
	}
};
