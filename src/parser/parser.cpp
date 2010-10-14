#include "parser.hpp"
#include "../compiler.hpp"

namespace Mirb
{
	Parser::Parser(SymbolPool &symbol_pool, MemoryPool &memory_pool, Compiler &compiler) : lexer(symbol_pool, memory_pool, compiler), current_block(0), compiler(compiler), memory_pool(memory_pool)
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
	
	/*
	 * These nodes uses no arguments and can be statically allocated.
	 */
	struct node Parser::nil_node = {0, 0, 0, 0, N_NIL, T_NONE, false};
	struct node Parser::self_node = {0, 0, 0, 0, N_SELF, T_NONE, false};
	struct node Parser::true_node = {0, 0, 0, 0, N_TRUE, T_NONE, false};
	struct node Parser::false_node = {0, 0, 0, 0, N_FALSE, T_NONE, false};

	struct node *Parser::alloc_scope(struct block **block_var, enum block_type type)
	{
		struct block *block = block_create(old_compiler, type);

		struct node *result = new (memory_pool) struct node;

		block->parent = current_block;
		block->owner = current_block ? current_block->owner : 0;

		result->left = (struct node *)block;
		result->type = N_SCOPE;
		result->unbalanced = false;

		current_block = block;

		*block_var = block;

		return result;
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

	struct node *Parser::parse_argument()
	{
		struct node *result = alloc_node(N_ARGUMENT);

		result->left = parse_expression();
		result->right = 0;

		if(matches(Lexeme::COMMA))
			result->right = parse_argument();
		else
			result->right = 0;

		return result;
	}

	struct node *Parser::parse_identifier()
	{
		rt_value symbol = (rt_value)lexer.lexeme.symbol;

		lexer.step();

		switch (lexeme())
		{
			ASSIGN_OPS
			{
				struct node *result;

				enum token_type op_type = (enum token_type)((size_t)lexeme() - (size_t)OP_TO_ASSIGN);

				lexer.step();

				if (rt_symbol_is_const(symbol))
					result = alloc_node(N_ASSIGN_CONST);
				else
					result = alloc_node(N_ASSIGN);

				result->right = alloc_node(N_BINARY_OP);
				result->right->op = op_type;

				if (rt_symbol_is_const(symbol))
				{
					result->left = &self_node;
					result->middle = (struct node *)symbol;
					result->right->left = alloc_node(N_CONST);
					result->right->left->left = &self_node;
					result->right->left->right = (struct node *)symbol;
				}
				else
				{
					result->left = (struct node *)scope_get(current_block, symbol);
					result->right->left = alloc_node(N_VAR);
					result->right->left->left = (struct node *)scope_get(current_block, symbol);
				}

				result->right->right = parse_expression();

				return result;
			}

			case T_ASSIGN:
			{
				struct node *result;

				lexer.step();

				if (rt_symbol_is_const(symbol))
				{
					result = alloc_node(N_ASSIGN_CONST);
					result->left = &self_node;
					result->middle = (struct node *)symbol;
				}
				else
				{
					result = alloc_node(N_ASSIGN);
					result->left = (struct node *)scope_get(current_block, symbol);
				}

				result->right = parse_expression();

				return result;
			}

			// Function call or local variable

			default:
				return parse_call(symbol, &self_node, true);
		}
	}

	struct node *Parser::parse_array_element()
	{
		struct node *result = alloc_node(N_ARRAY_ELEMENT);

		result->left = parse_expression();

		if(lexeme() == Lexeme::COMMA)
		{
			lexer.step();

			result->right  = parse_array_element();
		}
		else
			result->right = 0;

		return result;
	}

	struct node *Parser::parse_factor()
	{
		switch (lexeme())
		{
			case T_BEGIN:
				return parse_begin();

			case T_IF:
				return parse_if();

			case T_UNLESS:
				return parse_unless();

			case T_CASE:
				return parse_case();

			case T_CLASS:
				return parse_class();

			case T_MODULE:
				return parse_module();

			case T_DEF:
				return parse_method();

			case T_YIELD:
				return parse_yield();

			case T_RETURN:
				return parse_return();

			case T_BREAK:
				return parse_break();

			case T_NEXT:
				return parse_next();

			case T_REDO:
				return parse_redo();

			case T_SUPER:
				return parse_super();

			case T_SQUARE_OPEN:
				{
					struct node *result = alloc_node(N_ARRAY);

					lexer.step();

					if(lexeme() == Lexeme::SQUARE_CLOSE)
						result->left  = 0;
					else
						result->left = parse_array_element();

					match(Lexeme::SQUARE_CLOSE);

					return result;
				}

			case T_STRING:
				{
					struct node *result = alloc_node(N_STRING);

					result->left = (struct node *)lexer.lexeme.c_str;

					lexer.step();

					return result;
				}

			case T_STRING_START:
				{
					struct node *result = alloc_node(N_STRING_CONTINUE);

					result->left = 0;
					result->middle = (struct node *)lexer.lexeme.c_str;

					lexer.step();

					result->right = parse_statements();

					while(lexeme() == Lexeme::STRING_CONTINUE)
					{
						struct node *node = alloc_node(N_STRING_CONTINUE);

						node->left = result;
						node->middle = (struct node *)lexer.lexeme.c_str;

						lexer.step();

						node->right = parse_statements();

						result = node;
					}

					if(require(Lexeme::STRING_END))
					{
						struct node *node = alloc_node(N_STRING_START);

						node->left = result;
						node->right = (struct node *)lexer.lexeme.c_str;

						lexer.step();

						return node;
					}

					return result;
				}

			case T_SELF:
				{
					lexer.step();

					return &self_node;
				}

			case T_TRUE:
				{
					lexer.step();

					return &true_node;
				}

			case T_FALSE:
				{
					lexer.step();

					return &false_node;
				}

			case T_NIL:
				{
					lexer.step();

					return &nil_node;
				}

			case T_INTEGER:
				{
					struct node *result = alloc_node(N_NUMBER);
					
					std::string str = lexer.lexeme.string();
					
					result->left = (struct node *)(size_t)atoi(str.c_str());
					
					lexer.step();
					
					return result;
				}

			case T_IVAR:
				{
					rt_value symbol = (rt_value)lexer.lexeme.symbol;

					lexer.step();

					switch (lexeme())
					{
						ASSIGN_OPS
						{
							struct node *result;

							enum token_type op_type = (enum token_type)((size_t)lexeme() - (size_t)OP_TO_ASSIGN);

							lexer.step();

							result = alloc_node(N_IVAR_ASSIGN);

							result->right = alloc_node(N_BINARY_OP);
							result->right->op = op_type;
							result->right->left = alloc_node(N_IVAR);
							result->right->left->left = (struct node *)symbol;
							result->right->right = parse_expression();

							result->left = (struct node *)symbol;

							return result;
						}

						case T_ASSIGN:
						{
							struct node *result;

							lexer.step();

							result = alloc_node(N_IVAR_ASSIGN);
							result->left = (struct node *)symbol;
							result->right = parse_expression();

							return result;
						}

						default:
						{
							struct node *result = alloc_node(N_IVAR);

							result->left = (struct node *)symbol;

							return result;
						}
					}
				}

			case T_IDENT:
				return parse_identifier();

			case T_EXT_IDENT:
				return parse_call(0, &self_node, false);

			case Lexeme::PARENT_OPEN:
				{
					lexer.step();

					struct node *result = parse_statements();

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

	struct node *Parser::parse_unary()
	{
		switch(lexeme())
		{
			case Lexeme::LOGICAL_NOT:
				{
					struct node *result = alloc_node(N_NOT);

					lexer.step();

					result->left = parse_lookup_chain();

					return result;
				}

			case T_ADD:
			case T_SUB:
			{
				struct node *result = alloc_node(N_UNARY_OP);

				result->op = (enum token_type)((size_t)lexeme() + (size_t)OP_TO_UNARY);

				lexer.step();

				result->left = parse_lookup_chain();

				return result;
			}

			default:
				return parse_lookup_chain();
		}
	}

	struct node *Parser::parse_term()
	{
		struct node *result = parse_unary();

		while(lexeme() == Lexeme::MUL || lexeme() == Lexeme::DIV)
		{
			struct node *node = alloc_node(N_BINARY_OP);

			node->op = (enum token_type)lexeme();
			node->left = result;

			lexer.step();

			node->right = parse_unary();
			result = node;
		}

		return result;
	}

	struct node *Parser::parse_arithmetic()
	{
		struct node *result = parse_term();

		while (lexeme() == Lexeme::ADD || lexeme() == Lexeme::SUB)
		{
			struct node *node = alloc_node(N_BINARY_OP);

			node->op = (enum token_type)lexeme();
			node->left = result;

			lexer.step();

			node->right = parse_term();
			result = node;
		}

		return result;
	}

	struct node *Parser::parse_shift()
	{
		struct node *result = parse_arithmetic();

		while (lexeme() == Lexeme::LEFT_SHIFT || lexeme() == Lexeme::RIGHT_SHIFT)
		{
			struct node *node = alloc_node(N_BINARY_OP);

			node->op = (enum token_type)lexeme();
			node->left = result;

			lexer.step();

			node->right = parse_arithmetic();
			result = node;
		}

		return result;
	}

	bool Parser::is_equality_op()
	{
		switch(lexeme())
		{
			case T_EQUALITY:
			case T_CASE_EQUALITY:
			case T_NO_EQUALITY:
				return true;

			default:
				return false;
		}
	}

	struct node *Parser::parse_equality()
	{
		struct node *result = parse_shift();

		while(is_equality_op())
		{
			struct node *node;

			if(lexeme() == Lexeme::NO_EQUALITY)
				node = alloc_node( N_NO_EQUALITY);
			else
			{
				node = alloc_node( N_BINARY_OP);
				node->op = (enum token_type)lexeme();
			}

			node->left = result;

			lexer.step();

			node->right = parse_shift();
			result = node;
		}

		return result;
	}

	struct node *Parser::parse_boolean_and()
	{
		struct node *result = parse_equality();

		while(lexeme() == Lexeme::LOGICAL_AND)
		{
			struct node *node = alloc_node(N_BOOLEAN);

			node->op = (enum token_type)lexeme();
			node->left = result;

			lexer.step();

			node->right = parse_equality();
			result = node;
		}

		return result;
	}

	struct node *Parser::parse_boolean_or()
	{
		struct node *result = parse_boolean_and();

		while(lexeme() == Lexeme::LOGICAL_OR)
		{
			struct node *node = alloc_node(N_BOOLEAN);

			node->op = (enum token_type)lexeme();
			node->left = result;

			lexer.step();

			node->right = parse_boolean_and();
			result = node;
		}

		return result;
	}

	struct node *Parser::parse_expression()
	{
		return parse_ternary_if();
	}

	struct node *Parser::parse_low_boolean_unary()
	{
		if(lexeme() == Lexeme::KW_NOT)
		{
			struct node *result = alloc_node(N_NOT);

			lexer.step();

			result->left = parse_expression();

			return result;
		}
		else
			return parse_expression();
	}

	struct node *Parser::parse_low_boolean()
	{
		struct node *result = parse_low_boolean_unary();

		while(lexeme() == Lexeme::KW_AND || lexeme() == Lexeme::KW_OR)
		{
			struct node *node = alloc_node(N_BOOLEAN);

			node->op = (enum token_type)lexeme();
			node->left = result;

			lexer.step();

			node->right = parse_low_boolean_unary();
			result = node;
		}

		return result;
	}

	struct node *Parser::parse_statement()
	{
		return parse_conditional();
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

	struct node *Parser::parse_statements()
	{
		skip_seps();

		if(is_expression())
		{
			struct node *result = parse_statement();

			if (is_sep())
			{
				skip_seps();

				if(is_expression())
				{
					struct node *node = alloc_node(N_STATEMENTS);

					node->left = result;
					node->right = parse_statements();

					return node;
				}
			}

			return result;

		}
		else
			return &nil_node;
	}

	struct node *Parser::parse_main()
	{
		struct block *block;

		struct node *result = alloc_scope(&block, S_MAIN);

		block->owner = block;

		result->right = parse_statements();

		match(Lexeme::END);

		return result;
	}
};
