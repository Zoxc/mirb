#include "parser.hpp"

namespace Mirb
{
	bool Parser::is_jump_argument()
	{
		switch(lexeme())
		{
			case Lexeme::KW_UNLESS:
			case Lexeme::KW_IF:
			case Lexeme::KW_UNTIL:
			case Lexeme::KW_WHILE:
				return false;

			default:
				return is_expression();
		}
	}

	Tree::Node *Parser::parse_return()
	{
		auto range = capture();
		
		lexer.step();
		
		if(is_jump_argument())
			return raise_void(new (fragment) Tree::ReturnNode(range, parse_assignment(true, false)));
		else
			return raise_void(new (fragment) Tree::ReturnNode(range, new (fragment) Tree::NilNode));
	}

	Tree::Node *Parser::parse_next()
	{
		auto range = capture();
		
		lexer.step();
		
		if(is_jump_argument())
			return raise_void(new (fragment) Tree::NextNode(range, parse_assignment(true, false)));
		else
			return raise_void(new (fragment) Tree::NextNode(range, new (fragment) Tree::NilNode));
	}

	Tree::Node *Parser::parse_redo()
	{
		auto range = capture();
		
		lexer.step();
		
		return raise_void(new (fragment) Tree::RedoNode(range));
	}

	Tree::Node *Parser::parse_break()
	{
		auto range = capture();
		
		lexer.step();
		
		if(is_jump_argument())
			return raise_void(new (fragment) Tree::BreakNode(range, parse_assignment(true, false)));
		else
			return raise_void(new (fragment) Tree::BreakNode(range, new (fragment) Tree::NilNode));
	}

	Tree::Node *Parser::parse_exception_handlers(Tree::Node *block, Tree::VoidTrapper *trapper)
	{
		switch (lexeme())
		{
			case Lexeme::KW_ENSURE:
			case Lexeme::KW_RESCUE:
				break;
			
			default:
				return block;
				break;
		}
		
		auto result = new (fragment) Tree::HandlerNode;
		
		result->code = block;

		while(lexeme() == Lexeme::KW_RESCUE)
		{
			auto rescue = new (fragment) Tree::RescueNode;
			
			lexer.step();

			if(!is_sep() && lexeme() != Lexeme::KW_THEN && lexeme() != Lexeme::ASSOC)
				rescue->pattern = process_rhs(parse_multiple_expressions(true, true));
			else
				rescue->pattern = nullptr;
			
			if(matches(Lexeme::ASSOC))
			{
				SourceLoc range = lexer.lexeme;

				rescue->var = parse_operator_expression(true);

				lexer.lexeme.prev_set(&range);

				process_lhs(rescue->var, range);
			}
			else
				rescue->var = nullptr;

			parse_then_sep();
			
			rescue->group = parse_group();
			
			result->rescues.append(rescue);
		}
		
		if(matches(Lexeme::KW_ELSE))
			result->else_group = parse_group();

		if(matches(Lexeme::KW_ENSURE))
		{
			trapper->in_ensure = true;

			result->ensure_group = parse_group();
		}

		return result;
	}

	Tree::Node *Parser::parse_begin()
	{
		SourceLoc range = lexer.lexeme;

		lexer.step();

		Tree::Node *node;
		
		Tree::VoidTrapper *trapper = trap([&]{
			node = parse_group();
		});
		
		auto result = parse_exception_handlers(node, trapper);
		
		close_pair("begin", range, Lexeme::KW_END);
		
		return result;
	}

	void Parser::parse_then_sep()
	{
		switch (lexeme())
		{
			case Lexeme::KW_THEN:
				lexer.step();
				break;

			default:
				parse_sep();
		}
	}
	
	Tree::Node *Parser::parse_ternary_if(bool allow_multiples)
	{
		Tree::Node *result = parse_range(allow_multiples);
		
		if(lexeme() == Lexeme::QUESTION)
		{
			SourceLoc range = lexer.lexeme;

			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				lexer.step();

				skip_lines();
			
				auto node = new (fragment) Tree::IfNode;
			
				node->inverted = false;
			
				node->left = result;
				node->middle = parse_operator_expression(allow_multiples);
			
				close_pair("ternary if", range, Lexeme::COLON);

				skip_lines();
		
				node->right = parse_operator_expression(allow_multiples);

				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_conditional()
	{
		Tree::Node *result = parse_alias();
		
		if (lexeme() == Lexeme::KW_IF || lexeme() == Lexeme::KW_UNLESS)
		{
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::IfNode;
			
				node->inverted = lexeme() == Lexeme::KW_UNLESS;
			
				lexer.step();
			
				node->middle = result;
				node->left = typecheck(parse_tailing_loop());
				node->right = new (fragment) Tree::NilNode;
				
				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_high_rescue(bool allow_multiples, bool nested)
	{
		Tree::Node *result = process_rhs(parse_assignment(allow_multiples, nested));
		
		if(lexeme() == Lexeme::KW_RESCUE && result->type() != Tree::Node::MultipleExpressions)
		{
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::HandlerNode;

				node->code = result;

				auto rescue = new (fragment) Tree::RescueNode;
			
				lexer.step();

				rescue->pattern = nullptr;
				rescue->var = nullptr;
				rescue->group = typecheck(parse_operator_expression(true));
			
				node->rescues.append(rescue);

				return node;
			});
		}
		
		return result;
	}

	Tree::Node *Parser::parse_low_rescue()
	{
		Tree::Node *result = parse_conditional();
		
		if(lexeme() == Lexeme::KW_RESCUE)
		{
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::HandlerNode;
			
				node->code = result;

				auto rescue = new (fragment) Tree::RescueNode;
			
				lexer.step();

				rescue->pattern = nullptr;
				rescue->var = nullptr;
				rescue->group =  typecheck(parse_tailing_loop());
			
				node->rescues.append(rescue);

				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_tailing_loop()
	{
		Tree::Node *result;

		Tree::VoidTrapper *trapper = trap([&]{
			result = parse_low_rescue();
		});
		
		if (lexeme() == Lexeme::KW_WHILE || lexeme() == Lexeme::KW_UNTIL)
		{
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::LoopNode;
				
				trapper->target = node;
				
				node->inverted = lexeme() == Lexeme::KW_UNTIL;
			
				lexer.step();
			
				node->body = result;
				node->condition = typecheck(parse_tailing_loop());
				
				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_loop()
	{
		auto node = new (fragment) Tree::LoopNode;

		SourceLoc range = lexer.lexeme;
		
		node->inverted = lexeme() == Lexeme::KW_UNTIL;
		
		lexer.step();
		
		node->condition = typecheck(parse_expression());
				
		switch (lexeme())
		{
			case Lexeme::KW_DO:
				lexer.step();
				break;

			default:
				parse_sep();
		}

		Tree::VoidTrapper *trapper = trap([&]{
			node->body = parse_group();
		});

		trapper->target = node;
		
		close_pair(node->inverted ? "until loop" : "while loop", range, Lexeme::KW_END);
		
		return node;
	}

	Tree::Node *Parser::parse_unless()
	{
		SourceLoc range = lexer.lexeme;
		
		lexer.step();

		auto result = new (fragment) Tree::IfNode;
		
		result->inverted = true;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = parse_if_tail();
		
		close_pair("unless", range, Lexeme::KW_END);
		
		return result;
	}

	Tree::Node *Parser::parse_if_tail()
	{
		switch (lexeme())
		{
			case Lexeme::KW_ELSIF:
				{
					lexer.step();
					
					auto result = new (fragment) Tree::IfNode;
					
					result->inverted = false;
					
					result->left = parse_expression();
					
					parse_then_sep();
					
					result->middle = parse_group();
					result->right = parse_if_tail();
					
					return result;
				}

			case Lexeme::KW_ELSE:
				lexer.step();

				return parse_group();

			default:
				return new (fragment) Tree::NilNode;
		}
	}

	Tree::Node *Parser::parse_if()
	{
		SourceLoc range = lexer.lexeme;
		
		lexer.step();
		
		auto result = new (fragment) Tree::IfNode;
		
		result->inverted = false;
		
		result->left = parse_expression();
		
		parse_then_sep();
		
		result->middle = parse_group();
		result->right = parse_if_tail();
		
		close_pair("if", range, Lexeme::KW_END);
		
		return result;
	}

	Tree::Node *Parser::parse_case()
	{
		SourceLoc range = lexer.lexeme;
		
		lexer.step();

		auto result = new (fragment) Tree::CaseNode;

		if(!(lexeme() == Lexeme::KW_WHEN || lexeme() == Lexeme::KW_END || lexeme() == Lexeme::KW_ELSE || is_sep()))
			result->value = parse_expression();
		else
			result->value = nullptr;

		do
		{
			skip_seps();

			auto clause = new (fragment) Tree::CaseEntry;

			clause->range = lexer.lexeme;

			match(Lexeme::KW_WHEN);
			
			clause->pattern = process_rhs(parse_multiple_expressions(true, true));
			
			lexer.lexeme.prev_set(&clause->range);

			parse_then_sep();
			
			clause->group = parse_group();

			result->clauses.append(clause);
		}
		while(lexeme() == Lexeme::KW_WHEN);

		skip_seps();

		if(matches(Lexeme::KW_ELSE))
		{
			result->else_clause = parse_group();
		}
		else
			result->else_clause = nullptr;

		skip_seps();
		
		close_pair("case", range, Lexeme::KW_END);
		
		return result;
	}
};
