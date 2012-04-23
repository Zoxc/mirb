#include "parser.hpp"

namespace Mirb
{
	bool Parser::require_ident()
	{
		switch(lexeme())
		{
			case Lexeme::IDENT:
			case Lexeme::EXT_IDENT:
				return true;

			default:
				{
					expected(Lexeme::IDENT);
					
					return false;
				}
		}
	}

	Tree::BlockNode *Parser::parse_block()
	{
		bool curly;
		
		switch(lexeme())
		{
			case Lexeme::CURLY_OPEN:
				curly = true;
				break;
			
			case Lexeme::KW_DO:
				curly = false;
				break;
			
			default:
				return 0;
		}
		
		lexer.step();
		
		auto old_fragment = fragment;
		auto old_scope = scope;
		
		auto result = new (fragment) Tree::BlockNode;
		
		result->scope = allocate_scope(Tree::Scope::Closure);
		
		skip_lines();
		
		if(lexeme() == Lexeme::BITWISE_OR)
		{
			lexer.step();
			
			parse_parameters();
			
			match(Lexeme::BITWISE_OR);
		}
		
		result->scope->group = parse_group();
		
		fragment = old_fragment;
		scope = old_scope;
		
		match(curly ? Lexeme::CURLY_CLOSE : Lexeme::KW_END);
		
		return result;
	}

	bool Parser::has_arguments()
	{
		if(is_expression())
		{
			switch(lexeme())
			{
				case Lexeme::SQUARE_OPEN:
					return lexer.lexeme.whitespace;

				case Lexeme::ADD:
				case Lexeme::SUB:
					{
						if(lexer.lexeme.whitespace)
						{
							Lexer::Context context(lexer);
							
							lexer.step();
							
							bool result = !lexer.lexeme.whitespace;
							
							lexer.restore(context);
							
							return result;
						}
						else
							return false;
					}
					break;

				case Lexeme::CURLY_OPEN:
					return false;

				default:
					return true;
			}
		}
		else
			return false;
	}

	void Parser::parse_arguments(Tree::CountedNodeList &arguments, bool *parenthesis)
	{
		if(lexeme() == Lexeme::PARENT_OPEN && !lexer.lexeme.whitespace)
		{
			if(parenthesis)
				*parenthesis = true;
			
			lexer.step();

			if(lexeme() != Lexeme::PARENT_CLOSE)
				parse_arguments(arguments);
			
			match(Lexeme::PARENT_CLOSE);
		}
		else
		{
			if(parenthesis)
				*parenthesis = false;
			
			parse_arguments(arguments);
		}
	}

	Tree::Node *Parser::alloc_call_node(Tree::Node *object, Symbol *symbol, Range *range, bool has_args, bool can_be_var)
	{
		auto result = new (fragment) Tree::CallNode;
		
		result->object = object;
		result->method = symbol;
		result->can_be_var = can_be_var;
		result->range = range;
		
		if(has_args)
		{
			parse_arguments(result->arguments, 0);
			lexer.lexeme.prev_set(range);
		}
		
		result->block = parse_block();
		
		return result;
	}

	Tree::Node *Parser::parse_super()
	{
		lexer.step();
		
		auto result = new (fragment) Tree::SuperNode;
		
		bool parenthesis = false;
		
		if(has_arguments())
			parse_arguments(result->arguments, &parenthesis);
		
		result->block = parse_block();
		
		if(result->arguments.empty() && !parenthesis)
		{
			if(scope != scope->owner)
				scope->owner->zsupers.push(scope);
			
			result->pass_args = true;
		}
		else
			result->pass_args = false;
		
		return result;
	}

	Tree::Node *Parser::parse_yield()
	{
		auto range = new (fragment) Range(lexer.lexeme);

		lexer.step();

		auto child = new (fragment) Tree::VariableNode;
		
		if(!scope->block_parameter)
		{
			scope->block_parameter = scope->alloc_var<Tree::Parameter>();
			scope->block_parameter->name = 0;
		}
		
		child->var = scope->block_parameter;
		
		return alloc_call_node(child, Symbol::from_literal("call"), range, has_arguments());
	}

	Tree::Node *Parser::parse_call(Symbol *symbol, Tree::Node *child, Range *range, bool default_var)
	{
		mirb_debug_assert((symbol && (range != 0)) || (!symbol && range == 0));

		if(!symbol)
		{
			if(require_ident())
			{
				symbol = lexer.lexeme.symbol;

				range = new (fragment) Range(lexer.lexeme);
				
				lexer.step();
			}
			else
				range = 0;
		}
		
		bool local = false;
		
		if(symbol)
			local = scope->defined(symbol, true) != 0;
		
		bool has_args = has_arguments();
		bool can_be_var = default_var && !has_args;
		bool constant = symbol && is_constant(symbol);
		
		if(can_be_var && !has_args && (local || constant)) // Variable or constant
		{
			return parse_variable(symbol, range);
		}
		else // Call
		{
			return alloc_call_node(child, symbol, range, has_args, can_be_var && !constant);
		}
	}

	bool Parser::is_lookup()
	{
		switch(lexeme())
		{
			case Lexeme::SQUARE_OPEN:
			case Lexeme::SCOPE:
			case Lexeme::DOT:
				return true;
			
			default:
				return false;
		}
	}

	Tree::Node *Parser::parse_lookup(Tree::Node *child)
	{
		switch(lexeme())
		{
			case Lexeme::SCOPE:
				{
					lexer.lexeme.allow_keywords = false;
					
					lexer.step();
					
					lexer.lexeme.allow_keywords = true;

					if(require_ident())
					{
						Symbol *symbol = lexer.lexeme.symbol;

						auto range = new (fragment) Range(lexer.lexeme);

						lexer.step();

						bool has_args = has_arguments();

						if(has_args || !is_constant(symbol))
						{
							return alloc_call_node(child, symbol, range, has_arguments());
						}
						else
						{
							return new (fragment) Tree::ConstantNode(child, symbol, range);
						}
					}
					else
						return child;
				}

			case Lexeme::DOT:
				{
					lexer.lexeme.allow_keywords = false;

					lexer.step();

					lexer.lexeme.allow_keywords = true;

					return parse_call(0, child, 0, false);
				}

			case Lexeme::SQUARE_OPEN:
				{
					auto range = new (fragment) Range(lexer.lexeme);

					lexer.step();
					
					auto result = new (fragment) Tree::CallNode;
					
					result->method = Symbol::from_literal("[]");
					result->range = range; //TODO: Extend range back to child
					
					result->object = child;
					
					parse_arguments(result->arguments);
					
					result->block = parse_block();

					match(Lexeme::SQUARE_CLOSE);

					return result;
				}

			default:
				mirb_runtime_abort("Invalid lookup lexeme");
		}
	}

	Tree::Node *Parser::parse_lookup_chain()
	{
		Tree::Node *result = parse_factor();

		while(is_lookup())
		{
			Tree::Node *node = parse_lookup(result);

			result = node;
		}

		return result;
	}
};
