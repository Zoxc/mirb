#include "parser.hpp"

namespace Mirb
{
	Tree::Node *Parser::parse_class()
	{
		auto range = capture();

		lexer.step();

		auto result = new (fragment) Tree::ClassNode;
		
		result->range = range;

		parse_module_name(*result);

		if(lexeme() == Lexeme::LESS)
		{
			lexer.step();

			result->super = parse_expression();
		}
		else
			result->super = 0;
		
		lexer.lexeme.prev_set(range);

		parse_sep();
		
		allocate_scope(Tree::Scope::Class, [&]  {
			result->scope = scope;
			result->scope->group = parse_group();
		});		
		
		match(Lexeme::KW_END);
		
		return result;
	}
	
	void Parser::parse_module_name(Tree::ModuleNode &node)
	{
		auto range = capture();

		Tree::Node *result = parse_unary();

		lexer.lexeme.prev_set(range);
		
		node.name = nullptr;
		node.scoped = nullptr;

		if(!result)
			return;

		if(result->type() != Tree::Node::Constant)
		{
			report(*range, "Expected a constant");

			return;
		}
		
		auto constant = (Tree::ConstantNode *)result;

		node.top_scope = constant->top_scope;
		node.scoped = constant->obj;
		node.name = constant->name;
	}

	Tree::Node *Parser::parse_module()
	{
		auto range = capture();

		lexer.step();

		auto result = new (fragment) Tree::ModuleNode;

		result->range = range;

		parse_module_name(*result);
		
		lexer.lexeme.prev_set(range);

		allocate_scope(Tree::Scope::Module, [&]  {
			result->scope = scope;
			result->scope->group = parse_group();
		});		
		
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

		result->range = capture();

		Symbol *symbol;
		
		switch(lexeme())
		{
			case Lexeme::PARENT_OPEN:
			{
				lexer.step();
					
				if(lexeme() == Lexeme::PARENT_CLOSE)
					error("Expected expression");
				else
					result->singleton = parse_expression();
			
				match(Lexeme::PARENT_CLOSE);

				match(Lexeme::DOT);
			}
			break;
			
			case Lexeme::IVAR:
			{
				result->singleton = new (fragment) Tree::IVarNode(lexer.lexeme.symbol);
			
				lexer.step();

				match(Lexeme::DOT);
			}
			break;

			case Lexeme::GLOBAL:
			{
				result->singleton = new (fragment) Tree::GlobalNode(lexer.lexeme.symbol);
			
				lexer.step();

				match(Lexeme::DOT);
			}
			break;

			case Lexeme::IDENT:
			{
				Range *range = parse_method_name(symbol);

				if(lexeme() == Lexeme::DOT)
				{
					result->singleton = parse_variable(symbol, range);

					lexer.step();
				}
				else
				{
					result->singleton = nullptr;
					goto skip_name;
				}
			}
			break;

			default:
				result->singleton = nullptr;
				break;
		}
		
		parse_method_name(symbol);
skip_name:

		result->name = symbol;

		lexer.lexeme.prev_set(result->range);

		allocate_scope(Tree::Scope::Method, [&] {
			result->scope = scope;
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
		});		
		
		match(Lexeme::KW_END);
		
		return result;
	}
};
