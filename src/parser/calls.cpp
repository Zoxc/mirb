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

	Tree::Node *Parser::secure_block(Tree::BlockNode *result, Tree::InvokeNode *parent)
	{
		if(!result)
			return parent;
		
		if(result->scope->can_break)
		{
			parent->break_id = scope->break_targets++;
			result->scope->break_id = parent->break_id;
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

	void Parser::parse_arguments(Tree::NodeList &arguments, bool has_args, bool *parenthesis)
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

	Tree::Node *Parser::alloc_call_node(Tree::Node *object, Symbol *symbol, bool has_args)
	{
		auto result = new (fragment) Tree::CallNode;
		
		result->object = object;
		result->method = symbol;
		
		parse_arguments(result->arguments, has_args, 0);
		
		result->block = parse_block();
		
		return secure_block(result->block, result);
	}

	Tree::Node *Parser::parse_super()
	{
		lexer.step();
		
		auto result = new (fragment) Tree::SuperNode;
		
		Tree::Scope *owner = scope->owner;
		
		if(!owner->super_module_var)
		{
			owner->super_module_var = owner->alloc_var<Tree::NamedVariable>(Tree::Variable::Temporary);
			owner->super_module_var->name = 0;
				
			owner->super_name_var = owner->alloc_var<Tree::NamedVariable>(Tree::Variable::Temporary);
			owner->super_name_var->name = 0;
			
			if(scope != owner)
			{
				scope->require_var(owner, owner->super_module_var);
				scope->require_var(owner, owner->super_name_var);
			}
		}
		
		bool parenthesis = false;
		
		parse_arguments(result->arguments, has_arguments(), &parenthesis);
		
		result->block = parse_block();
		
		if(result->arguments.empty() && !parenthesis)
		{
			if(scope != owner)
				owner->zsupers.push(scope);
			
			result->pass_args = true;
		}
		else
			result->pass_args = false;
		
		return secure_block(result->block, result);
	}

	Tree::Node *Parser::parse_yield()
	{
		lexer.step();

		auto child = new (fragment) Tree::VariableNode;
		
		if(!scope->block_parameter)
		{
			scope->block_parameter = scope->alloc_var<Tree::Parameter>();
			scope->block_parameter->name = 0;
		}
		
		child->var = scope->block_parameter;
		
		return alloc_call_node(child, (Symbol *)rt_symbol_from_cstr("call"), has_arguments());
	}

	Tree::Node *Parser::parse_call(Symbol *symbol, Tree::Node *child, bool default_var)
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
			local = scope->defined(symbol, true) != 0;
		
		bool has_args = has_arguments();
		
		if(default_var && !has_args && (local || (symbol && is_constant(symbol)))) // Variable or constant
		{
			return parse_variable(symbol);
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

						lexer.step();

						bool has_args = has_arguments();

						if(has_args || !is_constant(symbol))
						{
							return alloc_call_node(child, symbol, has_arguments());
						}
						else
						{
							return new (fragment) Tree::ConstantNode(symbol);
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
					
					auto result = new (fragment) Tree::CallNode;
					
					result->method = (Symbol *)rt_symbol_from_cstr("[]");
					
					result->object = child;
					
					parse_arguments(result->arguments);
					
					result->block = parse_block();

					match(Lexeme::SQUARE_CLOSE);

					return secure_block(result->block, result);
				}

			default:
				debug_fail("Invalid lookup lexeme");
		}
	}

	Tree::Node *Parser::parse_lookup_tail(Tree::Node *tail)
	{
		if(!is_assignment_op())
			return tail;
		
		if(!tail)
		{
			lexer.step();
			parse_expression();
			return 0;
		}
		
		Tree::Node *handler = tail;
		
		switch(tail->type())
		{
			case Tree::Node::Constant:
			{
				Tree::ConstantNode *node = (Tree::ConstantNode *)tail;
				
				return parse_assignment(node);
			}
			
			case Tree::Node::Call:
			{
				Tree::CallNode *node = (Tree::CallNode *)tail;
				
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
					
					Tree::Node *argument = parse_expression();
					
					if(argument)
						node->arguments.append(argument);

					return handler;
				}
				else
				{
					// TODO: Save node->object in a temporary variable
					auto result = new (fragment) Tree::CallNode;
					
					result->object = node->object;
					result->method = mutated;
					result->block = 0;
					
					auto binary_op = new (fragment) Tree::BinaryOpNode;
					
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

	Tree::Node *Parser::parse_lookup_chain()
	{
		Tree::Node *result = parse_factor();

		while(is_lookup())
		{
			Tree::Node *node = parse_lookup(result);

			result = node;
		}

		return parse_lookup_tail(result);
	}
};
