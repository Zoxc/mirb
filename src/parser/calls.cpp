#include "parser.hpp"

namespace Mirb
{
	bool Parser::require_ident()
	{
		switch(lexeme())
		{
			case T_IDENT:
			case T_EXT_IDENT:
				return true;

			default:
				{
					expected(Lexeme::IDENT);
					
					return false;
				}
		}
	}

	struct node *Parser::parse_block()
	{
		bool curly;

		switch(lexeme())
		{
			case T_CURLY_OPEN:
				curly = true;
				break;

			case T_DO:
				curly = false;
				break;

			default:
				return 0;
		}

		lexer.step();

		struct block *block;

		struct node *result = alloc_scope(&block, S_CLOSURE);

		skip_lines();

		if(lexeme() == Lexeme::BITWISE_OR)
		{
			lexer.step();

			parse_parameter(block);

			match(Lexeme::BITWISE_OR);
		}

		result->right = parse_statements();

		current_block = block->parent;

		match(curly ? Lexeme::CURLY_CLOSE : Lexeme::KW_END);

		return result;
	}

	struct node *Parser::secure_block(struct node *result, struct node *parent)
	{
		if(!result)
			return parent;

		struct block *block = (struct block *)result->left;

		if(block->can_break)
		{
			struct node *handler = alloc_node(N_BREAK_HANDLER);

			block->break_id = current_block->break_targets++; // Assign a break id to the block and increase the break target count.

			handler->left = parent;
			handler->right = (struct node *)block;

			return handler;
		}

		return parent;
	}

	bool Parser::has_arguments()
	{
		if(is_expression())
		{
			switch(lexeme())
			{
				case T_SQUARE_OPEN:
					return lexer.lexeme.whitespace;

				case T_ADD:
				case T_SUB:
					{
						if(lexer.lexeme.whitespace)
						{
							Lexeme lexeme(lexer.lexeme);
							
							lexer.step();
							
							bool result = !lexer.lexeme.whitespace;
							
							lexer.lexeme = lexeme;
							
							return result;
						}
						else
							return false;
					}
					break;

				default:
					return true;
			}
		}
		else
			return false;
	}

	struct node *Parser::parse_arguments(bool has_args, bool *parenthesis)
	{
		if(has_args && lexeme() != Lexeme::CURLY_OPEN)
		{
			if(lexeme() == Lexeme::PARENT_OPEN && !lexer.lexeme.whitespace)
			{
				if(parenthesis)
					*parenthesis = true;

				struct node *result = 0;

				lexer.step();

				if(lexeme() != Lexeme::PARENT_CLOSE)
					result = parse_argument();

				match(Lexeme::PARENT_CLOSE);

				return result;
			}
			else
			{
				if(parenthesis)
					*parenthesis = false;

				return parse_argument();
			}
		}
		else
			return 0;
	}

	struct node *Parser::alloc_call_node(struct node *child, rt_value symbol, bool has_args)
	{
		struct node *result = alloc_node(N_CALL);

		result->left = child;
		result->middle = (struct node *)symbol;

		result->right = alloc_node(N_CALL_ARGUMENTS);
		result->right->left = parse_arguments(has_args, 0);
		result->right->right = parse_block();

		return secure_block(result->right->right, result);
	}

	struct node *Parser::parse_super()
	{
		lexer.step();

		struct node *result = alloc_node(N_SUPER);

		struct block *owner = current_block->owner;
		struct block *current = current_block;

		while(true)
		{
			if(!current->super_module_var)
			{
				current->super_module_var = block_get_var(current);
				current->super_name_var = block_get_var(current);
			}

			if(owner == current)
				break;

			current = current->parent;
		}

		bool parenthesis;

		result->left = parse_arguments(has_arguments(), &parenthesis);
		result->right = parse_block();

		if(result->left == 0 && !parenthesis)
		{
			if(current != owner)
				vec_push(blocks, &owner->zsupers, current);

			result->type = N_ZSUPER;
		}

		return secure_block(result->right, result);
	}

	struct node *Parser::parse_yield()
	{
		lexer.step();

		struct node *child = alloc_node(N_VAR);

		if(!current_block->block_parameter)
			current_block->block_parameter = scope_var(current_block);

		child->left = (struct node *)current_block->block_parameter;

		return alloc_call_node(child, rt_symbol_from_cstr("call"), has_arguments());
	}

	struct node *Parser::parse_call(rt_value symbol, struct node *child, bool default_var)
	{
		if(!symbol)
		{
			if(require_ident())
			{
				symbol = (rt_value)lexer.lexeme.symbol;

				lexer.step();
			}
		}

		bool local = false;

		if(symbol)
			local = scope_defined(current_block, symbol, true);

		bool has_args = has_arguments();

		if(default_var && !has_args && (local || (symbol && rt_symbol_is_const(symbol)))) // Variable or constant
		{
			if(local)
			{
				struct node *result = alloc_node(N_VAR);

				result->left = (struct node *)scope_get(current_block, symbol);

				return result;
			}
			else
			{
				struct node *result = alloc_node(N_CONST);

				result->left = child;
				result->right = (struct node *)symbol;

				return result;
			}
		}
		else // Call
		{
			return alloc_call_node(child, symbol, has_args);
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

	struct node *Parser::parse_lookup(struct node *child)
	{
		switch(lexeme())
		{
			case T_SCOPE:
				{
					lexer.lexeme.allow_keywords = false;
					
					lexer.step();
					
					lexer.lexeme.allow_keywords = true;

					if(require_ident())
					{
						rt_value symbol = (rt_value)lexer.lexeme.symbol;

						lexer.step();

						bool has_args = has_arguments();

						if(has_args || !rt_symbol_is_const(symbol))
						{
							return alloc_call_node(child, symbol, has_arguments());
						}
						else
						{
							struct node *result;

							result = alloc_node(N_CONST);

							result->left = child;
							result->right = (struct node *)symbol;

							return result;
						}
					}
					else
						return child;
				}

			case T_DOT:
				{
					lexer.lexeme.allow_keywords = false;

					lexer.step();

					lexer.lexeme.allow_keywords = true;

					return parse_call(0, child, false);
				}

			case T_SQUARE_OPEN:
				{
					lexer.step();

					struct node *result = alloc_node(N_ARRAY_CALL);

					result->left = child;
					result->middle = parse_argument();
					result->right = parse_block();

					match(Lexeme::SQUARE_CLOSE);

					return secure_block(result->right, result);
				}

			default:
				RT_ASSERT(0);
		}
	}

	struct node *Parser::parse_lookup_tail(struct node *tail)
	{
		if(tail)
		{
			struct node *handler = tail;

			if(handler->type == N_BREAK_HANDLER)
			{
				tail = handler->left;
			}

			switch(lexeme())
			{
				ASSIGN_OPS
					RT_ASSERT(0);

				case T_ASSIGN:
					{
						lexer.step();

						switch(tail->type)
						{
							case N_CONST:
								{
									struct node *result = alloc_node(N_ASSIGN_CONST);

									result->left = tail->left;
									result->middle = tail->right;
									result->right = parse_expression();

									if(handler != tail)
										handler->left = result;

									return handler;
								}

							case N_CALL:
								{
									if(tail->right->left || tail->right->right)
										break;

									if(tail->middle)
									{
										rt_value name = rt_string_from_symbol((rt_value)tail->middle);

										rt_concat_string(name, rt_string_from_cstr("="));

										tail->middle = (struct node *)rt_symbol_from_string(name);
									}

									tail->right = alloc_node(N_CALL_ARGUMENTS);
									tail->right->right = 0;
									tail->right->left = alloc_node(N_ARGUMENT);
									tail->right->left->left = parse_expression();
									tail->right->left->right = 0;

									return handler;
								}

							case N_ARRAY_CALL:
								{
									tail->type = N_CALL;

									struct node *block = tail->right;
									struct node *argument = tail->middle;

									tail->middle = (struct node *)rt_symbol_from_cstr("[]=");
									tail->right = alloc_node(N_CALL_ARGUMENTS);
									tail->right->right = block;
									tail->right->left = argument;

									while(argument->right)
									{
										argument = argument->right;
									}

									argument->right = alloc_node(N_ARGUMENT);
									argument->right->left = parse_expression();
									argument->right->right = 0;

									return handler;
								}

							default:
								break;

						}

						error("Cannot assign a value to an expression.");

						parse_expression();

						return 0;
					}
					break;

				default:
					return handler;
			}
		}
		else // The tail is unknown
		{
			switch(lexeme())
			{
				ASSIGN_OPS
				case T_ASSIGN:
					{
						lexer.step();
						parse_expression();
					}

				default:
					return 0;
			}
		}
	}

	struct node *Parser::parse_lookup_chain()
	{
		struct node *result = parse_factor();

		while(is_lookup())
		{
			struct node *node = parse_lookup(result);

			result = node;
		}

		return parse_lookup_tail(result);
	}
};
