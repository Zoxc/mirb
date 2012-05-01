#include "parser.hpp"
#include "../compiler.hpp"
#include "../document.hpp"

namespace Mirb
{
	Parser::Parser(SymbolPool &symbol_pool, MemoryPool memory_pool, Document *document) : lexer(symbol_pool, memory_pool, *this), document(*document), memory_pool(memory_pool), fragment(0), scope(0)
	{
	}
	
	Parser::~Parser()
	{
	}

	Range *Parser::capture()
	{
		return new (fragment) Range(lexer.lexeme);
	}
	
	void Parser::load()
	{
		mirb_debug_assert(document.data[document.length] == 0);

		lexer.load(document.data, document.length);
	}
	
	void Parser::report(Range &range, std::string text, Message::Severity severity)
	{
		new (memory_pool) StringMessage(*this, range, severity, text);
	}

	bool Parser::is_constant(Symbol *symbol)
	{
		const char_t c = *symbol->string.raw();
		
		return c >= 'A' && c <= 'Z';
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

	Range *Parser::parse_method_name(Symbol *&symbol)
	{
		Range *range = capture();

		switch(lexeme())
		{
			case Lexeme::IDENT:
			{
				symbol = lexer.lexeme.symbol;

				lexer.step();

				if(lexeme() == Lexeme::ASSIGN && lexer.lexeme.whitespace == false)
				{
					symbol = symbol_pool.get(symbol->string + "=");

					range->expand(lexer.lexeme);

					lexer.step();
				}

				return range;
			}
			
			case Lexeme::EXT_IDENT:
			{
				symbol = lexer.lexeme.symbol;

				lexer.step();

				return range;
			}
			
			case Lexeme::POWER:
			case Lexeme::MUL:
			case Lexeme::DIV:
			case Lexeme::MOD:
			case Lexeme::ADD:
			case Lexeme::SUB:
			case Lexeme::LEFT_SHIFT:
			case Lexeme::RIGHT_SHIFT:
			case Lexeme::AMPERSAND:
			case Lexeme::BITWISE_XOR:
			case Lexeme::BITWISE_OR:
			case Lexeme::COMPARE:
			case Lexeme::EQUALITY:
			case Lexeme::CASE_EQUALITY:
			case Lexeme::MATCHES:
			case Lexeme::NOT_MATCHES:
			case Lexeme::BITWISE_NOT:
			case Lexeme::LOGICAL_NOT:
			case Lexeme::UNARY_ADD:
			case Lexeme::UNARY_SUB:
			{
				symbol = symbol_pool.get(lexer.lexeme);

				lexer.step();

				return range;

			}

			case Lexeme::SQUARE_OPEN:
			{
				lexer.step();

				if(lexer.lexeme.whitespace == false && match(Lexeme::SQUARE_CLOSE))
				{
					if(lexer.lexeme.whitespace == false && lexeme() == Lexeme::ASSIGN)
					{
						symbol = symbol_pool.get("[]=");

						lexer.step();
					}
					else
					{
						symbol = symbol_pool.get("[]");
					}

					lexer.lexeme.prev_set(range);
				}
				else
					symbol = 0;

				return range;
			}
			
			default:
			{
				expected(Lexeme::IDENT);
					
				return 0;
			}
		}
	}
				
	Tree::Node *Parser::parse_variable(Symbol *symbol, Range *range)
	{
		if(is_constant(symbol))
		{
			auto result = new (fragment) Tree::ConstantNode;
			
			result->obj = nullptr;
			result->name = symbol;
			result->range = range;
			
			return result;
		}
		else
		{
			auto result = new (fragment) Tree::VariableNode;
			
			result->var = scope->get_var<Tree::NamedVariable>(symbol);
			
			return result;
		}
	}
	
	void Parser::parse_arguments(Tree::CountedNodeList &arguments)
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
			binary_op->range = capture();
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

	void Parser::process_string_entries(Tree::InterpolatedStringNode *root, StringData::Entry &tail)
	{
		auto &entries = lexer.lexeme.str->entries;

		for(auto entry: entries)
		{
			auto pair = new (fragment) Tree::InterpolatedPairNode;
			pair->string = entry->copy<Tree::Fragment>(fragment);

			switch(entry->type)
			{
				case Lexeme::IVAR:
					pair->group = new (fragment) Tree::IVarNode(entry->symbol);
					break;

				default:
					mirb_debug_abort("Unknown string type");
			}

			root->pairs.append(pair);
		}

		tail = lexer.lexeme.str->tail.copy<Tree::Fragment>(fragment);
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
				auto data = lexer.lexeme.str;

				if(data->entries.size())
				{
					auto result = new (fragment) Tree::InterpolatedStringNode;

					process_string_entries(result, result->tail);

					result->tail = data->tail.copy<Tree::Fragment>(fragment);

					lexer.step();

					return result;
				}
				else
				{
					auto result = new (fragment) Tree::StringNode;

					result->string = data->tail.copy<Tree::Fragment>(fragment);
					
					lexer.step();

					return result;
				}
			}

			case Lexeme::STRING_START:
			{
				auto result = new (fragment) Tree::InterpolatedStringNode;
				
				do
				{
					auto pair = new (fragment) Tree::InterpolatedPairNode;

					process_string_entries(result, pair->string);

					lexer.step();
					
					pair->group = parse_group();
					
					result->pairs.append(pair);
				}
				while(lexeme() == Lexeme::STRING_CONTINUE);
				
				if(require(Lexeme::STRING_END))
				{
					process_string_entries(result, result->tail);

					result->tail = lexer.lexeme.str->tail.copy<Tree::Fragment>(fragment);

					lexer.step();
				}
				else
					result->tail.length = 0;

				return result;
			}

			case Lexeme::KW_SELF:
			{
				lexer.step();
				
				scope->require_self = true;

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
				auto range = capture();

				lexer.step();
				
				scope->require_self = true;

				return parse_call(symbol, new (fragment) Tree::SelfNode, range, true); // Function call, constant or local variable
			}
			
			case Lexeme::UNARY_ADD:
			case Lexeme::UNARY_SUB:
			case Lexeme::EXT_IDENT:
			{
				auto symbol = symbol_pool.get(lexer.lexeme);
				auto range = capture();

				lexer.step();
				
				scope->require_self = true;

				return parse_call(symbol, new (fragment) Tree::SelfNode, range, false);
			}

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
				auto range = capture();

				lexer.step();

				auto node = parse_lookup_chain();

				lexer.lexeme.prev_set(range);
				
				return new (fragment) Tree::UnaryOpNode(Lexeme::LOGICAL_NOT, node, range);
			}
			
			case Lexeme::ADD:
			case Lexeme::SUB:
			{
				auto result = new (fragment) Tree::UnaryOpNode;
				
				result->op = Lexeme::operator_to_unary(lexeme());
				result->range = new (fragment) Range(lexer.lexeme);
				
				lexer.step();
				
				result->value = parse_lookup_chain();

				lexer.lexeme.prev_set(result->range);
				
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
		
		2, // COMPARE,
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
			Range *range = capture();
			
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
			node->range = range;

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
			binary_op->range = capture();
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
		{
			lexer.step();
			return parse_expression();
		}

		switch(input->type())
		{
			case Tree::Node::Call:
			{
				auto node = (Tree::CallNode *)input;
					
				if(!node->subscript)
					if(!node->arguments.empty() || node->block)
						break;
				
				if(node->can_be_var)
					return build_assignment(parse_variable(node->method, node->range));
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
						{
							node->arguments.append(argument);

							if(node->range)
								lexer.lexeme.prev_set(node->range);
						}

						return input;
					}
					else
					{
						// TODO: Save node->object in a temporary variable
						auto result = new (fragment) Tree::CallNode;
						
						result->object = node->object;
						result->method = mutated;
						result->block = 0;

						if(node->range)
							result->range = capture();
						else
							result->range = 0;
						
						auto binary_op = new (fragment) Tree::BinaryOpNode;
						
						binary_op->left = node;
						binary_op->range = capture();
						binary_op->op = Lexeme::assign_to_operator(lexeme());
						
						lexer.step();
						
						binary_op->right = parse_expression();

						lexer.lexeme.prev_set(result->range);
						
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

	Tree::Scope *Parser::parse_main()
	{
		fragment = *new Tree::Fragment::Base(Tree::Chunk::main_size);
		
		Tree::Scope *result;
		
		allocate_scope(Tree::Scope::Top, [&] {
			result = scope;
			scope->owner = scope;
			scope->group = parse_group();
		});
		
		match(Lexeme::END);
		
		return result;
	}
};
