#include "parser.hpp"
#include "../compiler.hpp"

namespace Mirb
{
	Parser::Parser(SymbolPool &symbol_pool, MemoryPool &memory_pool, CharArray &filename) : lexer(symbol_pool, memory_pool, *this), filename(filename), memory_pool(memory_pool), fragment(0), scope(0)
	{
	}
	
	Parser::~Parser()
	{
	}
	
	void Parser::load(const char_t *input, size_t length)
	{
		lexer.load(input, length);
	}
	
	void Parser::report(Range &range, std::string text, Message::Severity severity)
	{
		new StringMessage(*this, range, severity, text);
	}

	bool Parser::is_constant(Symbol *symbol)
	{
		const char c = *symbol->string.data;

		return c >= 'A' && c <= 'Z';
	}
	
	Tree::Scope *Parser::allocate_scope(Tree::Scope::Type type)
	{
		if(type == Tree::Scope::Closure || type == Tree::Scope::Method)
			fragment = new (gc) Tree::Fragment(fragment, Tree::Chunk::block_size);
		
		return scope = new (fragment) Tree::Scope(*fragment, scope, type);
	}
	
	void Parser::error(std::string text)
	{
		report(lexer.lexeme.dup(memory_pool), text);
	}

	void Parser::expected(Lexeme::Type what, bool skip)
	{
		error("Expected " + Lexeme::describe_type(what) + ", but found " + lexer.lexeme.describe());
		
		if(skip)
			lexer.step();
	}
	
	void Parser::unexpected(bool skip)
	{
		error("Unexpected " + lexer.lexeme.describe());

		if(skip)
			lexer.step();
	}
	
	void Parser::parse_sep()
	{
		switch (lexeme())
		{
			case Lexeme::LINE:
			case Lexeme::SEMICOLON:
				lexer.step();
				break;

			default:
				expected(Lexeme::SEMICOLON);
				break;
		}
	}
	
	Tree::Node *Parser::parse_variable(Symbol *symbol, Tree::Node *left)
	{
		if(is_constant(symbol))
		{
			auto result = new (fragment) Tree::ConstantNode;
			
			result->obj = left;
			result->name = symbol;
			
			return result;
		}
		else
		{
			auto result = new (fragment) Tree::VariableNode;
			
			result->var = scope->get_var<Tree::NamedVariable>(symbol);
			
			return result;
		}
	}
	
	void Parser::parse_arguments(Tree::NodeList &arguments)
	{
		do
		{
			auto node = parse_expression();
			
			if(node)
				arguments.append(node);
		}
		while(matches(Lexeme::COMMA));
	}
	
	bool Parser::is_assignment_op()
	{
		switch (lexeme())
		{
			case Lexeme::ASSIGN_POWER:
			case Lexeme::ASSIGN_ADD:
			case Lexeme::ASSIGN_SUB:
			case Lexeme::ASSIGN_MUL:
			case Lexeme::ASSIGN_DIV:
			case Lexeme::ASSIGN_MOD:
			case Lexeme::ASSIGN_LEFT_SHIFT:
			case Lexeme::ASSIGN_RIGHT_SHIFT:
			case Lexeme::ASSIGN_BITWISE_AND:
			case Lexeme::ASSIGN_BITWISE_XOR:
			case Lexeme::ASSIGN_BITWISE_OR:
			case Lexeme::ASSIGN_LOGICAL_AND:
			case Lexeme::ASSIGN_LOGICAL_OR:
			case Lexeme::ASSIGN:
				return true;
			
			default:
				return false;
		}
	}
	
	Tree::Node *Parser::parse_assignment(Tree::Node *variable)
	{
		if(lexeme() == Lexeme::ASSIGN)
		{
			auto result = new (fragment) Tree::AssignmentNode;
			
			lexer.step();
			
			result->op = Lexeme::ASSIGN;
			
			result->left = variable;
			
			result->right = parse_expression();
			
			return result;
		}
		else
		{
			auto result = new (fragment) Tree::AssignmentNode;
			auto binary_op = new (fragment) Tree::BinaryOpNode;

			result->op = Lexeme::ASSIGN;
			
			result->left = variable;
			result->right = binary_op;
			binary_op->left = variable;
			binary_op->op = Lexeme::assign_to_operator(lexeme());
			
			lexer.step();
			
			binary_op->right = parse_expression();
			
			return result;
		}
	}
	
	Tree::Node *Parser::parse_array()
	{
		auto result = new (fragment) Tree::ArrayNode;
		
		lexer.step();
		
		if(lexeme() != Lexeme::SQUARE_CLOSE)
		{
			do
			{
				auto element = parse_expression();
				
				result->entries.append(element);
			}
			while(matches(Lexeme::COMMA));
		}
		
		match(Lexeme::SQUARE_CLOSE);
		
		return result;
	}

	Tree::Node *Parser::parse_factor()
	{
		switch (lexeme())
		{
			case Lexeme::KW_BEGIN:
				return parse_begin();

			case Lexeme::KW_IF:
				return parse_if();

			case Lexeme::KW_UNLESS:
				return parse_unless();

			case Lexeme::KW_CASE:
				return parse_case();

			case Lexeme::KW_CLASS:
				return parse_class();

			case Lexeme::KW_MODULE:
				return parse_module();

			case Lexeme::KW_DEF:
				return parse_method();

			case Lexeme::KW_YIELD:
				return parse_yield();

			case Lexeme::KW_RETURN:
				return parse_return();

			case Lexeme::KW_BREAK:
				return parse_break();

			case Lexeme::KW_NEXT:
				return parse_next();

			case Lexeme::KW_REDO:
				return parse_redo();

			case Lexeme::KW_SUPER:
				return parse_super();

			case Lexeme::SQUARE_OPEN:
				return parse_array();

			case Lexeme::STRING:
			{
				auto result = new (fragment) Tree::StringNode;

				result->string = lexer.lexeme.c_str;

				lexer.step();

				return result;
			}

			case Lexeme::STRING_START:
			{
				auto result = new (fragment) Tree::InterpolatedStringNode;
					
				do
				{
					auto pair = new (fragment) Tree::InterpolatedPairNode;
						
					pair->string = lexer.lexeme.c_str;
						
					lexer.step();
						
					pair->group = parse_group();
						
					result->pairs.append(pair);
				}
				while(lexeme() == Lexeme::STRING_CONTINUE);
					
				if(require(Lexeme::STRING_END))
				{
					result->tail = lexer.lexeme.c_str;

					lexer.step();
				}
				else
					result->tail = 0;

				return result;
			}

			case Lexeme::KW_SELF:
			{
				lexer.step();

				return new (fragment) Tree::SelfNode;
			}

			case Lexeme::KW_TRUE:
			{
				lexer.step();

				return new (fragment) Tree::TrueNode;
			}

			case Lexeme::KW_FALSE:
			{
				lexer.step();

				return new (fragment) Tree::FalseNode;
			}

			case Lexeme::KW_NIL:
			{
				lexer.step();

				return new (fragment) Tree::NilNode;
			}

			case Lexeme::INTEGER:
			{
				auto result = new (fragment) Tree::IntegerNode;
					
				std::string str = lexer.lexeme.string();
					
				result->value = atoi(str.c_str());
					
				lexer.step();
					
				return result;
			}

			case Lexeme::IVAR:
			{
				Symbol *symbol = lexer.lexeme.symbol;
					
				lexer.step();
				
				return new (fragment) Tree::IVarNode(symbol);
			}

			case Lexeme::IDENT:
			{
				Symbol *symbol = lexer.lexeme.symbol;

				lexer.step();
				
				return parse_call(symbol, new (fragment) Tree::SelfNode, true); // Function call, constant or local variable
			}

			case Lexeme::EXT_IDENT:
				return parse_call(0, new (fragment) Tree::SelfNode, false);

			case Lexeme::PARENT_OPEN:
			{
				lexer.step();
					
				auto result = parse_group();
					
				match(Lexeme::PARENT_CLOSE);
					
				return result;
			}

			default:
			{
				error("Expected expression but found " + lexer.lexeme.describe());
					
				lexer.step();
					
				return 0;
			}
		}
	}

	Tree::Node *Parser::parse_unary()
	{
		switch(lexeme())
		{
			case Lexeme::LOGICAL_NOT:
			{
				lexer.step();
				
				return new (fragment) Tree::UnaryOpNode(Lexeme::LOGICAL_NOT, parse_lookup_chain());
			}
			
			case Lexeme::ADD:
			case Lexeme::SUB:
			{
				auto result = new (fragment) Tree::UnaryOpNode;
				
				result->op = Lexeme::operator_to_unary(lexeme());
				
				lexer.step();
				
				result->value = parse_lookup_chain();
				
				return result;
			}

			default:
				return parse_lookup_chain();
		}
	}

	size_t Parser::operator_precedences[] = {
		8, // MUL,
		8, // DIV,
		8, // MOD,
		
		7, // ADD,
		7, // SUB,
		
		6, // LEFT_SHIFT,
		6, // RIGHT_SHIFT,
		
		5, // AMPERSAND, // BITWISE_AND
		
		4, // BITWISE_XOR,
		4, // BITWISE_OR,
		
		1, // LOGICAL_AND,
		
		0, // LOGICAL_OR,
		
		3, // GREATER,
		3, // GREATER_OR_EQUAL,
		3, // LESS,
		3, // LESS_OR_EQUAL,
		
		2, // EQUALITY,
		2, // CASE_EQUALITY,
		2, // NO_EQUALITY,
		2, // MATCHES,
		2, // NOT_MATCHES,
	};

	bool Parser::is_precedence_operator(Lexeme::Type op)
	{
		return op >= Lexeme::precedence_operators_start && op <= Lexeme::precedence_operators_end;
	}

	Tree::Node *Parser::parse_precedence_operator()
	{
		return parse_precedence_operator(parse_unary(), 0);
	}

	Tree::Node *Parser::parse_precedence_operator(Tree::Node *left, size_t min_precedence)
	{
		while(true)
		{
			Lexeme::Type op = lexeme();
			
			if(!is_precedence_operator(op))
				break;
			
			size_t precedence = operator_precedences[op - Lexeme::precedence_operators_start];
			
			if(precedence < min_precedence)
				break;
			
			lexer.step();

			Tree::Node *right = parse_unary();

			while(true)
			{
				Lexeme::Type next_op = lexeme();

				if(!is_precedence_operator(next_op))
					break;

				size_t next_precedence = operator_precedences[next_op - Lexeme::precedence_operators_start];

				if(next_precedence <= precedence)
					break;

				right = parse_precedence_operator(right, next_precedence);
			}
			
			Tree::BinaryOpNode *node = new (fragment) Tree::BinaryOpNode;
			
			node->op = op;
			node->left = left;
			node->right = right;

			left = node;
		}

		return left;
	}
	
	Tree::Node *Parser::build_assignment(Tree::Node *left)
	{
		if(lexeme() == Lexeme::ASSIGN)
		{
			auto result = new (fragment) Tree::AssignmentNode;
			
			lexer.step();
			
			result->op = Lexeme::ASSIGN;
			
			result->left = left;
			
			result->right = parse_expression();
			
			return result;
		}
		else
		{
			auto result = new (fragment) Tree::AssignmentNode;
			auto binary_op = new (fragment) Tree::BinaryOpNode;

			result->op = Lexeme::ASSIGN;
			
			result->left = left;
			result->right = binary_op;
			binary_op->left = left;
			binary_op->op = Lexeme::assign_to_operator(lexeme());
			
			lexer.step();
			
			binary_op->right = parse_expression();
			
			return result;
		}
	};

	Tree::Node *Parser::parse_assignment()
	{
		Tree::Node *result = parse_ternary_if();
		
		while(is_assignment_op())
			result = process_assignment(result);
		
		return result;
	}

	Tree::Node *Parser::process_assignment(Tree::Node *input)
	{
		if(!input)
			return 0;

		switch(input->type())
		{
			case Tree::Node::Call:
			{
				auto node = (Tree::CallNode *)input;
					
				if(!node->arguments.empty() || node->block)
					break;
				
				if(node->can_be_var)
					return build_assignment(parse_variable(node->method, 0));
				else
				{
					Symbol *mutated = node->method;
					
					if(mutated)
						mutated = Symbol::from_char_array(node->method->string + "=");
					
					if(lexeme() == Lexeme::ASSIGN)
					{
						lexer.step();
						
						node->method = mutated;
						
						Tree::Node *argument = parse_expression();
						
						if(argument)
							node->arguments.append(argument);

						return input;
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

				break;
			};

			case Tree::Node::Variable:
			case Tree::Node::Constant: //TODO: Make sure constant's object is not re-evaluated
			case Tree::Node::IVar:
				return build_assignment(input);

			default:
				break;
		}
			
		error("Cannot assign a value to an expression.");
		
		lexer.step();

		parse_expression();

		return input;
	}
	
	Tree::Node *Parser::parse_boolean_unary()
	{
		if(lexeme() == Lexeme::KW_NOT)
		{
			lexer.step();
			return new (fragment) Tree::BooleanNotNode(parse_expression());
		}
		else
		{
			return parse_expression();
		}
	}

	Tree::Node *Parser::parse_boolean()
	{
		Tree::Node *result = parse_boolean_unary();
		
		while(lexeme() == Lexeme::KW_AND || lexeme() == Lexeme::KW_OR)
		{
			auto node = new (fragment) Tree::BooleanOpNode;
			
			node->op = lexeme();
			node->left = result;
			
			lexer.step();
			
			node->right = parse_boolean_unary();
			
			result = node;
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_group()
	{
		auto node = new (fragment) Tree::GroupNode;
		
		parse_statements(node->statements);
		
		return node;
	}
	
	bool Parser::is_sep()
	{
		return lexeme() == Lexeme::SEMICOLON || lexeme() == Lexeme::LINE;
	}
	
	void Parser::skip_seps()
	{
		while (is_sep())
			lexer.step();
	}
	
	void Parser::parse_statements(Tree::NodeList &list)
	{
		skip_seps();
		
		while(is_expression())
		{
			Tree::Node *node = parse_statement();
			
			if(node)
				list.append(node);
			
			if (!is_sep())
				return;
			
			skip_seps();
		}
	}

	Tree::Scope *Parser::parse_main(Tree::Fragment *fragment)
	{
		this->fragment = fragment;
		
		Tree::Scope *scope = allocate_scope(Tree::Scope::Top);
		
		scope->owner = scope;
		
		scope->group = parse_group();
		
		match(Lexeme::END);
		
		return scope;
	}
};
