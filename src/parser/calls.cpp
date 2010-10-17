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

	BlockNode *Parser::parse_block()
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
		
		auto result = new (memory_pool) BlockNode;
		
		struct block *block = alloc_scope(result->scope, S_CLOSURE);
		
		skip_lines();
		
		if(lexeme() == Lexeme::BITWISE_OR)
		{
			lexer.step();
			
			parse_parameters(block);
			
			match(Lexeme::BITWISE_OR);
		}
		
		result->scope.group = parse_group();
		
		current_block = block->parent;
		
		match(curly ? Lexeme::CURLY_CLOSE : Lexeme::KW_END);
		
		return result;
	}

	Node *Parser::secure_block(BlockNode *result, Node *parent)
	{
		if(!result)
			return parent;
		
		struct block *block = result->scope.block;
		
		if(block->can_break)
		{
			auto handler = new (memory_pool) BreakHandlerNode;
			
			block->break_id = current_block->break_targets++; // Assign a break id to the block and increase the break target count.
			
			handler->code = parent;
			handler->block = block;
			
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

				default:
					return true;
			}
		}
		else
			return false;
	}

	void Parser::parse_arguments(NodeList &arguments, bool has_args, bool *parenthesis)
	{
		if(!has_args || lexeme() == Lexeme::CURLY_OPEN)
			return;
		
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

	Node *Parser::alloc_call_node(Node *object, Symbol *symbol, bool has_args)
	{
		auto result = new (memory_pool) CallNode;
		
		result->object = object;
		result->method = symbol;
		
		parse_arguments(result->arguments, has_args, 0);
		
		result->block = parse_block();
		
		return secure_block(result->block, result);
	}

	Node *Parser::parse_super()
	{
		lexer.step();
		
		auto result = new (memory_pool) SuperNode;
		
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
		
		parse_arguments(result->arguments, has_arguments(), &parenthesis);
		
		result->block= parse_block();
		
		if(result->arguments.empty() && !parenthesis)
		{
			if(current != owner)
				vec_push(blocks, &owner->zsupers, current);
			
			result->pass_args = true;
		}
		else
			result->pass_args = false;
		
		return secure_block(result->block, result);
	}

	Node *Parser::parse_yield()
	{
		lexer.step();

		auto child = new (memory_pool) VariableNode;
		
		if(!current_block->block_parameter)
			current_block->block_parameter = scope_var(current_block);
		
		child->variable_type = VariableNode::Temporary;
		child->var = current_block->block_parameter;
		
		return alloc_call_node(child, (Symbol *)rt_symbol_from_cstr("call"), has_arguments());
	}

	Node *Parser::parse_call(Symbol *symbol, Node *child, bool default_var)
	{
		if(!symbol)
		{
			if(require_ident())
			{
				symbol = lexer.lexeme.symbol;
				
				lexer.step();
			}
		}
		
		bool local = false;
		
		if(symbol)
			local = scope_defined(current_block, (rt_value)symbol, true);
		
		bool has_args = has_arguments();
		
		if(default_var && !has_args && (local || (symbol && rt_symbol_is_const((rt_value)symbol)))) // Variable or constant
		{
			return new (memory_pool) VariableNode(*this, symbol);
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

	Node *Parser::parse_lookup(Node *child)
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

						lexer.step();

						bool has_args = has_arguments();

						if(has_args || !rt_symbol_is_const((rt_value)symbol))
						{
							return alloc_call_node(child, symbol, has_arguments());
						}
						else
						{
							return new (memory_pool) VariableNode(*this, symbol);
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

					return parse_call(0, child, false);
				}

			case Lexeme::SQUARE_OPEN:
				{
					lexer.step();
					
					auto result = new (memory_pool) CallNode;
					
					result->method = (Symbol *)rt_symbol_from_cstr("[]");
					
					result->object = child;
					
					parse_arguments(result->arguments);
					
					result->block = parse_block();

					match(Lexeme::SQUARE_CLOSE);

					return secure_block(result->block, result);
				}

			default:
				RT_ASSERT(0);
		}
	}

	Node *Parser::parse_lookup_tail(Node *tail)
	{
		if(!is_assignment_op())
			return tail;
		
		if(!tail)
		{
			lexer.step();
			parse_expression();
			return 0;
		}
		
		Node *handler = tail;
		
		if(tail->type() == Node::BreakHandler)
		{
			BreakHandlerNode *handler = (BreakHandlerNode *)tail;
			tail = handler->code;
		}
		
		switch(tail->type())
		{
			case Node::Variable:
			{
				VariableNode *node = (VariableNode *)tail;
				
				if(node->variable_type != VariableNode::Constant)
					return handler;
				
				return parse_assignment(node);
			}
			
			case Node::Call:
			{
				CallNode *node = (CallNode *)tail;
				
				if(!node->arguments.empty() || node->block)
					break;
				
				Symbol *mutated = node->method;
				
				if(mutated)
				{
					rt_value name = rt_string_from_symbol((rt_value)node->method);
					
					rt_concat_string(name, rt_string_from_cstr("="));
					
					mutated = (Symbol *)rt_symbol_from_string(name);
				}
				
				if(lexeme() == Lexeme::ASSIGN)
				{
					lexer.step();
					
					node->method = mutated;
					
					Node *argument = parse_expression();
					
					if(argument)
						node->arguments.append(argument);

					return handler;
				}
				else
				{
					// TODO: Save node->object in a temporary variable
					auto result = new (memory_pool) CallNode;
					
					result->object = node->object;
					result->method = mutated;
					result->block = 0;
					
					auto binary_op = new (memory_pool) BinaryOpNode;
					
					binary_op->left = node;
					binary_op->op = Lexeme::assign_to_operator(lexeme());
					
					lexer.step();
					
					binary_op->right = parse_expression();
					
					result->arguments.append(binary_op);
					
					return result;
				}
				
			}
			
			default:
				break;
		}
		
		error("Cannot assign a value to an expression.");
		
		lexer.step();

		parse_expression();

		return 0;
	}

	Node *Parser::parse_lookup_chain()
	{
		Node *result = parse_factor();

		while(is_lookup())
		{
			Node *node = parse_lookup(result);

			result = node;
		}

		return parse_lookup_tail(result);
	}
};
