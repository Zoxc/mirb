#include "parser.hpp"
#include "../compiler.hpp"

namespace Mirb
{
	Parser::Parser(SymbolPool &symbol_pool, MemoryPool &memory_pool, Compiler &compiler) : lexer(symbol_pool, memory_pool, compiler), memory_pool(memory_pool), current_block(0), compiler(compiler)
	{
		old_compiler = compiler_create();
	}
	
	Parser::~Parser()
	{
		compiler_destroy(old_compiler);
	}
	
	void Parser::error(std::string text)
	{
		compiler.report(lexer.lexeme.dup(memory_pool), text);
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
	
	struct block *Parser::alloc_scope(Scope &scope, enum block_type type)
	{
		struct block *block = block_create(old_compiler, type);
		
		block->parent = current_block;
		block->owner = current_block ? current_block->owner : 0;
		
		scope.block = block;
		
		current_block = block;
		
		return block;
	}

	struct block *Parser::scope_defined(struct block *block, rt_value name, bool recursive)
	{
		if(hash_get(block, block->variables, name) != hash_end(block->variables))
			return block;

		if(recursive)
		{
			while (block->type == S_CLOSURE)
			{
				block = block->parent;

				if(hash_get(block, block->variables, name) != hash_end(block->variables))
					return block;
			}
		}

		return 0;
	}

	struct variable *Parser::scope_var(struct block *block)
	{
		struct variable *var = new (memory_pool) struct variable;
		var->owner = block;
		var->type = V_LOCAL;
		var->name = 0;
		var->index = block->var_count[V_LOCAL]++;

		return var;
	}

	struct variable *Parser::scope_declare_var(struct block *block, rt_value name)
	{
		hash_iter_t i = hash_get(block, block->variables, name);

		if (i != hash_end(block->variables))
			return hash_value(block->variables, i);

		int ret;

		i = hash_put(block, block->variables, name, &ret);

		RT_ASSERT(ret);

		struct variable *var = new (memory_pool) struct variable;
		var->owner = block;
		var->type = V_LOCAL;
		var->name = name;

		hash_value(block->variables, i) = var;

		return var;
	}

	struct variable *Parser::scope_define(struct block *block, rt_value name)
	{
		if(scope_defined(block, name, false))
			return hash_value(block->variables, hash_get(block, block->variables, name));
		else
			return scope_declare_var(block, name);
	}

	struct variable *Parser::scope_get(struct block *block, rt_value name)
	{
		struct block *defined_block = scope_defined(block, name, true);

		if(defined_block == block)
		{
			return hash_value(block->variables, hash_get(block, block->variables, name));
		}
		else if(defined_block)
		{
			struct variable *var;

			hash_iter_t i = hash_get(block, defined_block->variables, name);

			var = hash_value(defined_block->variables, i);

			block_require_var(block, var);

			return var;
		}
		else
			return scope_declare_var(block, name);
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
	
	void Parser::parse_arguments(NodeList &arguments)
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
	
	Node *Parser::parse_assignment(VariableNode *variable)
	{
		if(lexeme() == Lexeme::ASSIGN)
		{
			auto result = new (memory_pool) AssignmentNode();
			
			lexer.step();
			
			result->op = Lexeme::ASSIGN;
			
			result->left = variable;
			
			result->right = parse_expression();
			
			return result;
		}
		else
		{
			auto result = new (memory_pool) AssignmentNode();
			auto binary_op = new (memory_pool) BinaryOpNode();
			
			result->left = variable;
			result->right = binary_op;
			binary_op->left = variable;
			binary_op->op = Lexeme::assign_to_operator(lexeme());
			
			lexer.step();
			
			binary_op->right = parse_expression();
			
			return result;
		}
	}
	
	Node *Parser::parse_identifier()
	{
		Symbol *symbol = lexer.lexeme.symbol;

		lexer.step();
		
		if(is_assignment_op())
		{
			auto variable = new (memory_pool) VariableNode(*this, symbol);
			
			return parse_assignment(variable);
		}
		else
			return parse_call(symbol,  new (memory_pool) SelfNode, true); // Function call or local variable
	}

	Node *Parser::parse_array()
	{
		auto result = new (memory_pool) ArrayNode;
		
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

	Node *Parser::parse_factor()
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
					auto result = new (memory_pool) StringNode;

					result->string = lexer.lexeme.c_str;

					lexer.step();

					return result;
				}

			case Lexeme::STRING_START:
				{
					auto result = new (memory_pool) InterpolatedStringNode;
					
					do
					{
						auto pair = new (memory_pool) InterpolatedPairNode;
						
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

					return new (memory_pool) SelfNode;
				}

			case Lexeme::KW_TRUE:
				{
					lexer.step();

					return new (memory_pool) TrueNode;
				}

			case Lexeme::KW_FALSE:
				{
					lexer.step();

					return new (memory_pool) FalseNode;
				}

			case Lexeme::KW_NIL:
				{
					lexer.step();

					return new (memory_pool) NilNode;
				}

			case Lexeme::INTEGER:
				{
					auto result = new (memory_pool) IntegerNode;
					
					std::string str = lexer.lexeme.string();
					
					result->value = atoi(str.c_str());
					
					lexer.step();
					
					return result;
				}

			case Lexeme::IVAR:
				{
					Symbol *symbol = lexer.lexeme.symbol;
					
					lexer.step();
					
					auto variable = new (memory_pool) VariableNode(VariableNode::Instance, symbol);
					
					if(is_assignment_op())
						return parse_assignment(variable);
					else
						return variable;
				}

			case Lexeme::IDENT:
				return parse_identifier();

			case Lexeme::EXT_IDENT:
				return parse_call(0, new (memory_pool) SelfNode, false);

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

	Node *Parser::parse_unary()
	{
		switch(lexeme())
		{
			case Lexeme::LOGICAL_NOT:
			{
				lexer.step();
				
				return new (memory_pool) UnaryOpNode(Lexeme::LOGICAL_NOT, parse_lookup_chain());
			}
			
			case Lexeme::ADD:
			case Lexeme::SUB:
			{
				auto result = new (memory_pool) UnaryOpNode;
				
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
		0, // MUL,
		0, // DIV,
		0, // MOD,
		
		1, // ADD,
		1, // SUB,
		
		2, // LEFT_SHIFT,
		2, // RIGHT_SHIFT,
		
		3, // AMPERSAND, // BITWISE_AND
		
		4, // BITWISE_XOR,
		4, // BITWISE_OR,
		
		7, // LOGICAL_AND,
		
		8, // LOGICAL_OR,
		
		5, // GREATER,
		5, // GREATER_OR_EQUAL,
		5, // LESS,
		5, // LESS_OR_EQUAL,
		
		6, // EQUALITY,
		6, // CASE_EQUALITY,
		6, // NO_EQUALITY,
		6, // MATCHES,
		6, // NOT_MATCHES,
	};

	bool Parser::is_precedence_operator(Lexeme::Type op)
	{
		return op >= Lexeme::precedence_operators_start && op <= Lexeme::precedence_operators_end;
	}

	Node *Parser::parse_precedence_operator()
	{
		return parse_precedence_operator(parse_unary(), 0);
	}

	Node *Parser::parse_precedence_operator(Node *left, size_t min_precedence)
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

			Node *right = parse_unary();

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
			
			BinaryOpNode *node = new (memory_pool) BinaryOpNode;
			
			node->op = op;
			node->left = left;
			node->right = right;

			left = node;
		}

		return left;
	}

	Node *Parser::parse_boolean_unary()
	{
		if(lexeme() == Lexeme::KW_NOT)
		{
			return new (memory_pool) UnaryOpNode(Lexeme::LOGICAL_NOT, parse_expression());
		}
		else
		{
			return parse_expression();
		}
	}

	Node *Parser::parse_boolean()
	{
		Node *result = parse_boolean_unary();
		
		while(lexeme() == Lexeme::KW_AND || lexeme() == Lexeme::KW_OR)
		{
			auto node = new (memory_pool) BinaryOpNode;
			
			node->op = lexeme();
			node->left = result;
			
			lexer.step();
			
			node->right = parse_boolean_unary();
			
			result = node;
		}
		
		return result;
	}
	
	Node *Parser::parse_group()
	{
		auto node = new (memory_pool) GroupNode;
		
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
	
	void Parser::parse_statements(NodeList &list)
	{
		skip_seps();
		
		while(is_expression())
		{
			Node *node = parse_statement();
			
			if(node)
				list.append(node);
			
			if (!is_sep())
				return;
			
			skip_seps();
		}
	}

	void Parser::parse_main(Scope &scope)
	{
		struct block *block = alloc_scope(scope, S_MAIN);

		block->owner = block;
		
		scope.group = parse_group();
		
		match(Lexeme::END);
	}
};
