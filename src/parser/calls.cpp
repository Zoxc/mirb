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

	Tree::Scope *Parser::parse_block(bool allowed)
	{
		bool curly;

		auto range = capture();
		
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
		
		Tree::Scope *result;
		
		allocate_scope(Tree::Scope::Closure, [&] {
			result = scope;
			
			skip_lines();
		
			if(lexeme() == Lexeme::BITWISE_OR)
			{
				SourceLoc block_range = lexer.lexeme;
		
				lexer.step();

				skip_lines();

				if(lexeme() != Lexeme::BITWISE_OR)
					parse_parameters(true);
			
				close_pair("block parameters", block_range, Lexeme::BITWISE_OR);
			}
			else if(lexeme() == Lexeme::LOGICAL_OR)
				lexer.step();

			lexer.lexeme.prev_set(range);
			scope->range = range;
		
			scope->group = parse_group();
		});
		
		close_pair("block", *result->range, curly ? Lexeme::CURLY_CLOSE : Lexeme::KW_END);

		if(!allowed)
			report(*result->range, "Unexpected block after block argument");
		
		return result;
	}

	bool Parser::has_arguments()
	{
		if(is_jump_argument() || lexeme() == Lexeme::AMPERSAND)
		{
			switch(lexeme())
			{
				case Lexeme::SQUARE_OPEN:
				case Lexeme::SCOPE:
					return lexer.lexeme.whitespace;

				case Lexeme::ADD:
				case Lexeme::SUB:
				case Lexeme::MUL:
				case Lexeme::MOD:
				case Lexeme::DIV:
				case Lexeme::QUESTION:
				case Lexeme::AMPERSAND:
				case Lexeme::LEFT_SHIFT:
					return lexer.lexeme.whitespace && !lexer.whitespace_after();
					
				case Lexeme::ASSIGN_DIV:
				case Lexeme::ASSIGN_MOD:
				case Lexeme::CURLY_OPEN:
					return false;

				default:
					return true;
			}
		}
		else
			return false;
	}

	void Parser::parse_arguments(Tree::InvokeNode *node, bool *parenthesis)
	{
		if(lexeme() == Lexeme::PARENT_OPEN && !lexer.lexeme.whitespace)
		{
			if(parenthesis)
				*parenthesis = true;
			
			SourceLoc range = lexer.lexeme;

			lexer.step();
			
			skip_lines();
					
			if(lexeme() != Lexeme::PARENT_CLOSE)
				parse_argument_list(node, Lexeme::PARENT_CLOSE);
			
			skip_lines();

			close_pair("call arguments", range, Lexeme::PARENT_CLOSE);
		}
		else
		{
			if(parenthesis)
				*parenthesis = false;
			
			parse_argument_list(node, Lexeme::NONE);
		}
	}

	Tree::Node *Parser::alloc_call_node(Tree::Node *object, Symbol *symbol, SourceLoc *range, bool has_args, bool can_be_var)
	{
		auto result = new (fragment) Tree::CallNode;
		
		result->object = object;
		result->method = symbol;
		result->can_be_var = can_be_var;
		result->range = range;
		
		if(has_args)
		{
			parse_arguments(result, 0);

			if(range)
				lexer.lexeme.prev_set(range);
		}
		
		result->scope = parse_block(result->block_arg == 0);
				
		return result;
	}

	Tree::Node *Parser::parse_super()
	{
		auto range = capture();
		lexer.step();
		
		auto result = new (fragment) Tree::SuperNode;
		
		bool parenthesis = false;
		
		if(has_arguments())
			parse_arguments(result, &parenthesis);

		lexer.lexeme.prev_set(range);
		result->range = range;
		
		result->scope = parse_block(result->block_arg == 0);
		
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
		auto range = capture();

		lexer.step();

		auto child = new (fragment) Tree::VariableNode;
		
		if(!scope->block_parameter)
		{
			scope->block_parameter = scope->alloc_var<Tree::Parameter>();
			scope->block_parameter->name = 0;
		}
		
		child->var = (Tree::NamedVariable *)scope->block_parameter;
		
		return alloc_call_node(child, Symbol::get("call"), range, has_arguments());
	}

	Tree::Node *Parser::parse_call(Symbol *symbol, Tree::Node *child, SourceLoc *range, bool default_var)
	{
		mirb_debug_assert((symbol && (range != 0)) || (!symbol && range == 0));

		if(!symbol)
			range = parse_method_name(symbol);
		
		bool has_args = has_arguments();
		bool can_be_var = default_var && !has_args;
		bool constant = symbol && is_constant(symbol);

		if(can_be_var && !has_args) // Variable or constant
		{
			bool local = false;

			if(symbol)
				local = scope->defined(symbol, true) != 0;
		
			if(local || constant)
				return parse_variable(symbol, range);
		}

		// Call
		return alloc_call_node(child, symbol, range, has_args, can_be_var && !constant);
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
					
					skip_lines();
					
					lexer.lexeme.allow_keywords = true;

					if(require_ident())
					{
						Symbol *symbol = lexer.lexeme.symbol;

						auto range = capture();

						lexer.step();

						bool has_args = has_arguments();

						if(has_args || !is_constant(symbol))
						{
							return alloc_call_node(child, symbol, range, has_args);
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
					
					skip_lines();
					
					lexer.lexeme.allow_keywords = true;

					if(lexeme() == Lexeme::PARENT_OPEN)
						return parse_call(symbol_pool.get("call"), child, capture(), false);
					else
						return parse_call(0, child, 0, false);
				}

			case Lexeme::SQUARE_OPEN:
				{
					auto range = capture();

					lexer.step();
					
					skip_lines();
					
					auto result = new (fragment) Tree::CallNode;
					
					result->method = Symbol::get("[]");
					result->range = range; //TODO: Extend range back to child
					result->subscript = true;
					
					result->object = child;

					if(lexeme() != Lexeme::SQUARE_CLOSE)
						parse_argument_list(result, Lexeme::SQUARE_CLOSE);

					close_pair("square bracket arguments", *range, Lexeme::SQUARE_CLOSE);

					lexer.lexeme.prev_set(range);

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
			typecheck(result, [&](Tree::Node *result) {
				return parse_lookup(result);
			});
		}

		return result;
	}
};
