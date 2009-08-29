#include "../runtime/classes/symbol.h"
#include "parser.h"
#include "calls.h"
#include "control_blocks.h"
#include "structures.h"


bool scope_defined(scope_t *scope, rt_value name, bool recursive)
{
	if(kh_get(scope, scope->variables, name) != kh_end(scope->variables))
		return true;

	if(recursive)
	{
		while (scope->type == S_CLOSURE)
		{
			scope = scope->owner;

			if(kh_get(scope, scope->variables, name) != kh_end(scope->variables))
				return true;
		}
	}

	return false;
}

variable_t *scope_declare_var(scope_t *scope, rt_value name, variable_type type)
{
	khiter_t k = kh_get(scope, scope->variables, name);

	if (k != kh_end(scope->variables))
		return kh_value(scope->variables, k);

	int ret;

	k = kh_put(scope, scope->variables, name, &ret);

	assert(ret);

	variable_t *var = malloc(sizeof(variable_t));
	var->type = type;
	var->name = name;
	var->index = scope->var_count[type];
	var->real = 0;

	scope->var_count[type] += 1;

	kh_value(scope->variables, k) = var;

	return var;
}

variable_t *scope_define(scope_t *scope, rt_value name, rt_value type, bool recursive)
{
	if(scope_defined(scope, name, false))
	{
		return kh_value(scope->variables, kh_get(scope, scope->variables, name));
	}
	else if(scope_defined(scope, name, true) && recursive)
	{
		variable_t *result = scope_declare_var(scope, name, V_UPVAL);

		scope_t *temp_scope = scope;

		while (temp_scope->type == S_CLOSURE)
			temp_scope = temp_scope->owner;

		variable_t *real = kh_value(temp_scope->variables, kh_get(scope, temp_scope->variables, name));
		result->real = real;

		while (scope->type == S_CLOSURE)
		{
			variable_t *var = scope_declare_var(scope, name, V_UPVAL);
			var->real = real;

			scope = scope->owner;
		}

		return result;
	}
	else
	{
		return scope_declare_var(scope, name, type);
	}
}

void parse_sep(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_LINE:
		case T_SEP:
			next(parser);
			break;

		default:
			parser->err_count++;
			printf("Excepted seperator but found %s\n", token_type_names[parser_current(parser)]);
	}
}

struct node *parse_argument(struct parser *parser)
{
	struct node *result = alloc_node(N_ARGUMENT);

	result->left = parse_expression(parser);
	result->right = 0;

	if(matches(parser, T_COMMA))
		result->right = parse_argument(parser);
	else
		result->right = 0;

	return result;
}

struct node *parse_identifier(struct parser *parser)
{
	rt_value symbol = rt_symbol_from_parser(parser);

	next(parser);

	switch (parser_current(parser))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
		{
			struct node *result;

			token_type op_type = parser_current(parser) - OP_TO_ASSIGN;

			next(parser);

			if (rt_symbol_is_const(symbol))
				result = alloc_node(N_ASSIGN_CONST);
			else
				result = alloc_node(N_ASSIGN);

			result->right = alloc_node(N_EXPRESSION);
			result->right->op = op_type;

			if (rt_symbol_is_const(symbol))
			{
				result->left = alloc_node(N_SELF);
				result->middle = (void *)symbol;
				result->right->left = alloc_node(N_CONST);
				result->right->left->left = alloc_node(N_SELF);
				result->right->left->right = (void *)symbol;
			}
			else
			{
				result->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);
				result->right->left = alloc_node(N_VAR);
				result->right->left->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);
			}

			result->right->right = parse_expression(parser);

			return result;
		}

		case T_ASSIGN:
		{
			struct node *result;

			next(parser);

			if (rt_symbol_is_const(symbol))
			{
				result = alloc_node(N_ASSIGN_CONST);
				result->left = alloc_node(N_SELF);
				result->middle = (void *)symbol;
			}
			else
			{
				result = alloc_node(N_ASSIGN);
				result->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);
			}

			result->right = parse_expression(parser);

			return result;
		}

		// Function call or local variable

		default:
			return parse_call(parser, symbol, alloc_node(N_SELF), true);
	}
}

struct node *parse_factor(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_IF:
			return parse_if(parser);

		case T_UNLESS:
			return parse_unless(parser);

		case T_CASE:
			return parse_case(parser);

		case T_CLASS:
			return parse_class(parser);

		case T_MODULE:
			return parse_module(parser);

		case T_DEF:
			return parse_method(parser);

		case T_YIELD:
			return parse_yield(parser);

		case T_STRING:
			{
			    struct node *result = alloc_node(N_STRING);

				result->left = (void *)parser_token(parser)->start;

				next(parser);

				return result;
			}

		case T_STRING_START:
			{
			    struct node *result = alloc_node(N_STRING_CONTINUE);

				result->left = 0;
				result->middle = (void *)parser_token(parser)->start;

				next(parser);

				result->right = parse_statements(parser);

				while(parser_current(parser) == T_STRING_CONTINUE)
				{
					struct node *node = alloc_node(N_STRING_CONTINUE);

					node->left = result;
					node->middle = (void *)parser_token(parser)->start;

					next(parser);

					node->right = parse_statements(parser);

					result = node;
				}

				if(require(parser, T_STRING_END))
				{
					struct node *node = alloc_node(N_STRING_START);

					node->left = result;
					node->right = (void *)parser_token(parser)->start;

					next(parser);

					return node;
				}

				return result;
			}

		case T_SELF:
			{
				next(parser);

				return alloc_node(N_SELF);
			}

		case T_TRUE:
			{
				next(parser);

				return alloc_node(N_TRUE);
			}

		case T_FALSE:
			{
				next(parser);

				return alloc_node(N_FALSE);
			}

		case T_NIL:
			{
				next(parser);

				return alloc_node(N_NIL);
			}

		case T_NUMBER:
			{
			    struct node *result = alloc_node(N_NUMBER);

			    char *text = get_token_str(parser_token(parser));

			    result->left = (void* )atoi(text);

			    free(text);

			    next(parser);

				return result;
			}

		case T_IDENT:
			return parse_identifier(parser);

		case T_PARAM_OPEN:
			{
				next(parser);

				struct node *result = parse_statements(parser);

				match(parser, T_PARAM_CLOSE);

				return result;
			}

		default:
			{
				parser->err_count++;
				printf("Excepted expression but found %s\n", token_type_names[parser_current(parser)]);

				next(parser);

				return 0;
			}
	}
}

struct node *parse_unary(struct parser *parser)
{
    if(parser_current(parser) == T_ADD || parser_current(parser) == T_SUB)
    {
    	struct node *result = alloc_node(N_UNARY);

		result->op = parser_current(parser);

		next(parser);

		result->left = parse_lookup_chain(parser);;

		return result;
    }
    else
		return parse_lookup_chain(parser);
}

struct node *parse_term(struct parser *parser)
{
	struct node *result = parse_unary(parser);

    while(parser_current(parser) == T_MUL || parser_current(parser) == T_DIV)
    {
    	struct node *node = alloc_node(N_TERM);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_unary(parser);
		result = node;
    }

	return result;
}

struct node *parse_arithmetic(struct parser *parser)
{
	struct node *result = parse_term(parser);

    while (parser_current(parser) == T_ADD || parser_current(parser) == T_SUB)
    {
    	struct node *node = alloc_node(N_EXPRESSION);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_term(parser);
		result = node;
    }

    return result;
}

struct node *parse_expression(struct parser *parser)
{
	return parse_ternary_if(parser);
}

struct node *parse_statement(struct parser *parser)
{
	return parse_conditional(parser);
}

static inline bool is_sep(struct parser *parser)
{
	return parser_current(parser) == T_SEP || parser_current(parser) == T_LINE;
}

static inline void skip_seps(struct parser *parser)
{
	while (is_sep(parser))
		next(parser);
}

struct node *parse_statements(struct parser *parser)
{
	skip_seps(parser);

	if(is_expression(parser))
	{
		struct node *result = parse_statement(parser);

		if (is_sep(parser))
		{
			skip_seps(parser);

			if(is_expression(parser))
			{
				struct node *node = alloc_node(N_STATEMENTS);

				node->left = result;
				node->right = parse_statements(parser);

				return node;
			}
		}

		return result;

	}
	else
		return alloc_nil_node();
}

struct node *parse_main(struct parser *parser)
{
	scope_t *scope;

	struct node *result = alloc_scope(parser, &scope, S_MAIN);

	result->right = parse_statements(parser);

	match(parser, T_EOF);

	return result;
}

