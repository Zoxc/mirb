#include "parser.hpp"

namespace Mirb
{
	Tree::Node *Parser::parse_class()
	{
		auto range = capture();

		lexer.step();

		auto result = new (fragment) Tree::ClassNode;
		
		result->range = range;

		if(matches(Lexeme::LEFT_SHIFT))
		{
			result->singleton = parse_expression();
			result->super = nullptr;
			result->name = nullptr;
		}
		else
		{
			result->singleton = nullptr;

			parse_module_name(*result);

			if(lexeme() == Lexeme::LESS)
			{
				lexer.step();

				result->super = parse_expression();
			}
			else
				result->super = nullptr;
		}

		lexer.lexeme.prev_set(range);

		parse_sep();
		
		allocate_scope(Tree::Scope::Class, [&]  {
			result->scope = scope;
			result->scope->group = parse_group();
		});		
		
		close_pair("class", *range, Lexeme::KW_END);
		
		return result;
	}
	
	void Parser::parse_module_name(Tree::ModuleNode &node)
	{
		auto range = capture();

		Tree::Node *result = typecheck(parse_unary());

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
		
		close_pair("module", *range, Lexeme::KW_END);
		
		return result;
	}

	bool Parser::is_parameter()
	{
		switch(lexeme())
		{
			case Lexeme::PARENT_OPEN:
			case Lexeme::AMPERSAND:
			case Lexeme::MUL:
			case Lexeme::IDENT:
				return true;

			default:
				return false;
		}
	}
	
	void Parser::parse_parameter(bool block)
	{
		if(lexeme() == Lexeme::PARENT_OPEN)
		{
			SourceLoc parent = lexer.lexeme;

			lexer.step();

			SourceLoc range;

			auto group = parse_multiple_expressions(true, false);

			lexer.lexeme.prev_set(&range);

			process_lhs(group, range, true);

			auto parameter = scope->alloc_var<Tree::Parameter>();

			parameter->range = range;
			parameter->node = group;
			parameter->parameter_group = true;

			scope->parameters.push(parameter);

			close_pair("parameter group", parent, Lexeme::PARENT_CLOSE);
				
			return;
		}
			
		SourceLoc range = lexer.lexeme;

		bool block_parameter = matches(Lexeme::AMPERSAND);
		bool array_parameter = matches(Lexeme::MUL);

		if(block_parameter && array_parameter)
			error("A parameter cannot be both the block and the array parameter");

		Tree::Parameter *parameter = nullptr;

		if(lexeme() == Lexeme::IDENT)
		{
			Symbol *symbol = lexer.lexeme.symbol;
			range.expand(lexer.lexeme);
			
			if(scope->defined(symbol, false))
				error("Variable " + lexer.lexeme.string() + " already defined");
			else
			{
				parameter = scope->define<Tree::Parameter>(symbol);
				parameter->range = range;
			}
				
			lexer.step();
		}
		else if(array_parameter)
		{
			parameter = scope->alloc_var<Tree::Parameter>();
			parameter->range = range;
			parameter->discard = true;
		}
		else
			error("Expected a parameter name");

		if(scope->block_parameter && !scope->block_parameter->reported)
		{
			scope->block_parameter->reported = true;
			report(scope->block_parameter->range, "Block parameters must be last in the parameter list");
		}
			
		if(matches(Lexeme::ASSIGN))
		{
			auto node = block ? parse_lookup_chain() : parse_operator_expression(false);

			if(parameter)
			{
				parameter->node = node;
				parameter->has_default_value = true;
			}

			if(array_parameter || block_parameter)
				report(range, (array_parameter ? "Array" : "Block") + std::string(" parameters cannot have default values"));
		}
		else if(!array_parameter && !block_parameter)
		{
			auto prev = scope->parameters.find([&](Tree::Parameter *param) -> bool { return param->has_default_value; }, nullptr);

			if(prev && !prev->reported)
			{
				prev->reported = true;
				report(prev->range, "Parameters with default values must come after regular parameters");
			}
		}

		if(array_parameter)
		{
			if(scope->array_parameter)
				report(range, "You can only receive the remaining arguments in one parameter");
			else
				scope->array_parameter = parameter;
		}
		else if(block_parameter)
		{
			if(scope->block_parameter)
				report(range, "You can only receive the block in one parameter");
			else
				scope->block_parameter = parameter;
		}
		else if (parameter)
			scope->parameters.push(parameter);
	}

	void Parser::parse_parameters(bool block)
	{
		parse_parameter(block);

		while(lexeme() == Lexeme::COMMA)
		{
			SourceLoc comma = lexer.lexeme;

			step_lines();
					
			if(lexeme() == Lexeme::COMMA)
			{
				SourceLoc error;

				while(lexeme() == Lexeme::COMMA)
				{
					step_lines();
				}

				report(error, "Unexpected multiple commas");
			}
			
			if(is_parameter())
				parse_parameter(block);
			else
			{
				auto parameter = scope->alloc_var<Tree::Parameter>();

				parameter->range = comma;
				parameter->discard = true;

				if(scope->array_parameter)
					report(comma, "Cannot have a trailing comma and an array parameter");
				else
					scope->array_parameter = parameter;
			}
		}
	}

	Tree::Node *Parser::parse_method()
	{
		lexer.lexeme.allow_keywords = false;

		auto range = capture();
		
		lexer.step();
		
		lexer.lexeme.allow_keywords = true;
		
		auto result = new (fragment) Tree::MethodNode;

		result->range = range;

		Symbol *symbol;
		
		switch(lexeme())
		{
			case Lexeme::PARENT_OPEN:
			{
				lexer.step();
					
				skip_lines();
					
				if(lexeme() == Lexeme::PARENT_CLOSE)
					error("Expected expression");
				else
					result->singleton = parse_expression();
			
				match(Lexeme::PARENT_CLOSE);

				match(Lexeme::DOT);
			}
			break;
			
			case Lexeme::CVAR:
			{
				result->singleton = new (fragment) Tree::CVarNode(lexer.lexeme.symbol);
			
				lexer.step();

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
				SourceLoc *range = parse_method_name(symbol);

				if(lexeme() == Lexeme::DOT)
				{
					if(symbol == Symbol::get("self"))
						result->singleton = new (fragment) Tree::SelfNode;
					else
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
			scope->range = result->range;

			if(lexeme() == Lexeme::PARENT_OPEN)
			{
				SourceLoc parent_open = lexer.lexeme;

				step_lines();
					
				if(lexeme() != Lexeme::PARENT_CLOSE)
					parse_parameters(false);
			
				close_pair("method parameters", parent_open, Lexeme::PARENT_CLOSE);
			}
			else
			{
				if(is_parameter())
					parse_parameters(false);
			
				parse_sep();
			}
		
			Tree::Node *node;
		
			Tree::VoidTrapper *trapper = trap([&]{
				node = parse_group();
			});
			
			result->scope->group = parse_exception_handlers(node, trapper);
		});		
		
		close_pair("method", *range, Lexeme::KW_END);
		
		return result;
	}
};
