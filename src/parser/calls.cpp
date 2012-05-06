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

	Tree::BlockNode *Parser::parse_block(bool allowed)
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
		
		auto result = new (fragment) Tree::BlockNode;
		
		allocate_scope(Tree::Scope::Closure, [&] {
			result->scope = scope;
			
			skip_lines();
		
			if(lexeme() == Lexeme::BITWISE_OR)
			{
				lexer.step();
			
				parse_parameters();
			
				match(Lexeme::BITWISE_OR);
			}

			lexer.lexeme.prev_set(range);
			scope->range = range;
		
			result->scope->group = parse_group();
		});
		
		match(curly ? Lexeme::CURLY_CLOSE : Lexeme::KW_END);

		if(!allowed)
			report(*result->scope->range, "Unexpected block after block argument");
		
		return result;
	}

	bool Parser::has_arguments()
	{
		if(is_expression() || lexeme() == Lexeme::AMPERSAND)
		{
			switch(lexeme())
			{
				case Lexeme::SQUARE_OPEN:
				case Lexeme::SCOPE:
					return lexer.lexeme.whitespace;

				case Lexeme::ADD:
				case Lexeme::SUB:
				case Lexeme::MUL:
				case Lexeme::QUESTION:
				case Lexeme::COLON:
				case Lexeme::AMPERSAND:
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

	void Parser::parse_arguments(Tree::InvokeNode *node, bool *parenthesis)
	{
		if(lexeme() == Lexeme::PARENT_OPEN && !lexer.lexeme.whitespace)
		{
			if(parenthesis)
				*parenthesis = true;
			
			lexer.step();

			if(lexeme() != Lexeme::PARENT_CLOSE)
				parse_arguments(node);
			
			skip_lines();

			match(Lexeme::PARENT_CLOSE);
		}
		else
		{
			if(parenthesis)
				*parenthesis = false;
			
			parse_arguments(node);
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
			parse_arguments(result, 0);
			lexer.lexeme.prev_set(range);
		}
		
		result->block = parse_block(result->block_arg == 0);
				
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
		
		result->block = parse_block(result->block_arg == 0);
		
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
		
		return alloc_call_node(child, Symbol::from_literal("call"), range, has_arguments());
	}

	Tree::Node *Parser::parse_call(Symbol *symbol, Tree::Node *child, Range *range, bool default_var)
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
					
					auto result = new (fragment) Tree::CallNode;
					
					result->method = Symbol::from_literal("[]");
					result->range = range; //TODO: Extend range back to child
					result->subscript = true;
					
					result->object = child;

					if(lexeme() != Lexeme::SQUARE_CLOSE)
						parse_arguments(result);

					result->block = nullptr;

					match(Lexeme::SQUARE_CLOSE);
					
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
