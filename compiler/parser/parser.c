#include "../../runtime/classes/symbol.h"
#include "parser.h"
#include "calls.h"
#include "control_flow.h"
#include "structures.h"

bool scope_defined(scope_t *scope, rt_value name, bool recursive)
{
	if(kh_get(scope, scope->variables, name) != kh_end(scope->variables))
		return true;

	if(recursive)
	{
		while (scope->type == S_CLOSURE)
		{
			scope = scope->parent;

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

	RT_ASSERT(ret);

	variable_t *var = parser_alloc(scope->parser, sizeof(variable_t));
	var->type = type;
	var->name = name;
	var->index = scope->var_count[type];
	var->real = 0;

	scope->var_count[type] += 1;

	kh_value(scope->variables, k) = var;

	return var;
}

variable_t *scope_define(scope_t *scope, rt_value name, variable_type type, bool recursive)
{
	if(scope_defined(scope, name, false))
	{
		return kh_value(scope->variables, kh_get(scope, scope->variables, name));
	}
	else if(scope_defined(scope, name, true) && recursive)
	{
		variable_t *result = scope_declare_var(scope, name, V_UPVAL);

		scope = scope->parent;

		scope_t *temp_scope = scope;
		variable_t *real;

		while(1)
		{
			khiter_t k = kh_get(scope, temp_scope->variables, name);

			if(k != kh_end(temp_scope->variables) && kh_value(temp_scope->variables, k)->type != V_UPVAL)
			{
				real = kh_value(temp_scope->variables, k);
				break;
			}

			temp_scope = temp_scope->parent;
		}

		result->real = real;

		while (scope != temp_scope)
		{
			variable_t *var = scope_declare_var(scope, name, V_UPVAL);
			var->real = real;

			scope = scope->parent;
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
			PARSER_ERROR(parser, "Expected seperator but found %s", token_type_names[parser_current(parser)]);
	}
}

node_t *parse_argument(struct parser *parser)
{
	node_t *result = alloc_node(parser, N_ARGUMENT);

	result->left = parse_expression(parser);
	result->right = 0;

	if(matches(parser, T_COMMA))
		result->right = parse_argument(parser);
	else
		result->right = 0;

	return result;
}

node_t *parse_identifier(struct parser *parser)
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
			node_t *result;

			token_type op_type = parser_current(parser) - OP_TO_ASSIGN;

			next(parser);

			if (rt_symbol_is_const(symbol))
				result = alloc_node(parser, N_ASSIGN_CONST);
			else
				result = alloc_node(parser, N_ASSIGN);

			result->right = alloc_node(parser, N_BINARY_OP);
			result->right->op = op_type;

			if (rt_symbol_is_const(symbol))
			{
				result->left = alloc_node(parser, N_SELF);
				result->middle = (void *)symbol;
				result->right->left = alloc_node(parser, N_CONST);
				result->right->left->left = alloc_node(parser, N_SELF);
				result->right->left->right = (void *)symbol;
			}
			else
			{
				result->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);
				result->right->left = alloc_node(parser, N_VAR);
				result->right->left->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);
			}

			result->right->right = parse_expression(parser);

			return result;
		}

		case T_ASSIGN:
		{
			node_t *result;

			next(parser);

			if (rt_symbol_is_const(symbol))
			{
				result = alloc_node(parser, N_ASSIGN_CONST);
				result->left = alloc_node(parser, N_SELF);
				result->middle = (void *)symbol;
			}
			else
			{
				result = alloc_node(parser, N_ASSIGN);
				result->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);
			}

			result->right = parse_expression(parser);

			return result;
		}

		// Function call or local variable

		default:
			return parse_call(parser, symbol, alloc_node(parser, N_SELF), true);
	}
}

node_t *parse_array_element(struct parser *parser)
{
	node_t *result = alloc_node(parser, N_ARRAY_ELEMENT);

	result->left = parse_expression(parser);

	if(parser_current(parser) == T_COMMA)
	{
		next(parser);

		result->right  = parse_array_element(parser);
	}
	else
		result->right = 0;

	return result;
}

node_t *parse_factor(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_BEGIN:
			return parse_begin(parser);

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

		case T_RETURN:
			return parse_return(parser);

		case T_SQUARE_OPEN:
			{
			    node_t *result = alloc_node(parser, N_ARRAY);

			    next(parser);

			    if(parser_current(parser) == T_SQUARE_CLOSE)
					result->left  = 0;
				else
					result->left = parse_array_element(parser);

				match(parser, T_SQUARE_CLOSE);

				return result;
			}

		case T_STRING:
			{
			    node_t *result = alloc_node(parser, N_STRING);

				result->left = (void *)parser_token(parser)->start;

				next(parser);

				return result;
			}

		case T_STRING_START:
			{
			    node_t *result = alloc_node(parser, N_STRING_CONTINUE);

				result->left = 0;
				result->middle = (void *)parser_token(parser)->start;

				next(parser);

				result->right = parse_statements(parser);

				while(parser_current(parser) == T_STRING_CONTINUE)
				{
					node_t *node = alloc_node(parser, N_STRING_CONTINUE);

					node->left = result;
					node->middle = (void *)parser_token(parser)->start;

					next(parser);

					node->right = parse_statements(parser);

					result = node;
				}

				if(require(parser, T_STRING_END))
				{
					node_t *node = alloc_node(parser, N_STRING_START);

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

				return alloc_node(parser, N_SELF);
			}

		case T_TRUE:
			{
				next(parser);

				return alloc_node(parser, N_TRUE);
			}

		case T_FALSE:
			{
				next(parser);

				return alloc_node(parser, N_FALSE);
			}

		case T_NIL:
			{
				next(parser);

				return alloc_node(parser, N_NIL);
			}

		case T_NUMBER:
			{
			    node_t *result = alloc_node(parser, N_NUMBER);

			    char *text = get_token_str(parser_token(parser));

			    result->left = (void* )atoi(text);

			    next(parser);

				return result;
			}

		case T_IVAR:
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
						node_t *result;

						token_type op_type = parser_current(parser) - OP_TO_ASSIGN;

						next(parser);

						result = alloc_node(parser, N_IVAR_ASSIGN);

						result->right = alloc_node(parser, N_BINARY_OP);
						result->right->op = op_type;
						result->right->left = alloc_node(parser, N_IVAR);
						result->right->left->left = (void *)symbol;
						result->right->right = parse_expression(parser);

						result->left = (void *)symbol;

						return result;
					}

					case T_ASSIGN:
					{
						node_t *result;

						next(parser);

						result = alloc_node(parser, N_IVAR_ASSIGN);
						result->left = (void *)symbol;
						result->right = parse_expression(parser);

						return result;
					}

					default:
					{
						node_t *result = alloc_node(parser, N_IVAR);

						result->left = (void *)symbol;

						return result;
					}
				}
			}

		case T_IDENT:
			return parse_identifier(parser);

		case T_EXT_IDENT:
			return parse_call(parser, 0, alloc_node(parser, N_SELF), false);

		case T_PARAM_OPEN:
			{
				next(parser);

				node_t *result = parse_statements(parser);

				match(parser, T_PARAM_CLOSE);

				return result;
			}

		default:
			{
				PARSER_ERROR(parser, "Expected expression but found %s", token_type_names[parser_current(parser)]);

				next(parser);

				return 0;
			}
	}
}

node_t *parse_unary(struct parser *parser)
{
    if(parser_current(parser) == T_ADD || parser_current(parser) == T_SUB)
    {
    	node_t *result = alloc_node(parser, N_UNARY_OP);

		result->op = parser_current(parser) + OP_TO_UNARY;

		next(parser);

		result->left = parse_lookup_chain(parser);;

		return result;
    }
    else
		return parse_lookup_chain(parser);
}

node_t *parse_term(struct parser *parser)
{
	node_t *result = parse_unary(parser);

    while(parser_current(parser) == T_MUL || parser_current(parser) == T_DIV)
    {
    	node_t *node = alloc_node(parser, N_BINARY_OP);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_unary(parser);
		result = node;
    }

	return result;
}

node_t *parse_arithmetic(struct parser *parser)
{
	node_t *result = parse_term(parser);

    while (parser_current(parser) == T_ADD || parser_current(parser) == T_SUB)
    {
    	node_t *node = alloc_node(parser, N_BINARY_OP);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_term(parser);
		result = node;
    }

    return result;
}

node_t *parse_boolean_unary(struct parser *parser)
{
    if(parser_current(parser) == T_NOT_SIGN)
    {
    	node_t *result = alloc_node(parser, N_NOT);

		next(parser);

		result->left = parse_arithmetic(parser);

		return result;
    }
    else
		return parse_arithmetic(parser);
}

static inline bool is_equality_op(struct parser *parser)
{
	switch(parser_current(parser))
	{
		case T_EQUALITY:
		case T_CASE_EQUALITY:
		case T_NO_EQUALITY:
			return true;

		default:
			return false;
	}
}

node_t *parse_equality(struct parser *parser)
{
	node_t *result = parse_boolean_unary(parser);

    while(is_equality_op(parser))
    {
    	node_t *node;

    	if(parser_current(parser) == T_NO_EQUALITY)
    		node = alloc_node(parser,  N_NO_EQUALITY);
		else
		{
			node = alloc_node(parser,  N_BINARY_OP);
			node->op = parser_current(parser);
		}

		node->left = result;

		next(parser);

		node->right = parse_boolean_unary(parser);
		result = node;
    }

    return result;
}

node_t *parse_boolean_and(struct parser *parser)
{
	node_t *result = parse_equality(parser);

    while(parser_current(parser) == T_AND_BOOLEAN)
    {
    	node_t *node = alloc_node(parser, N_BOOLEAN);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_equality(parser);
		result = node;
    }

    return result;
}

node_t *parse_boolean_or(struct parser *parser)
{
	node_t *result = parse_boolean_and(parser);

    while(parser_current(parser) == T_OR_BOOLEAN)
    {
    	node_t *node = alloc_node(parser, N_BOOLEAN);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_boolean_and(parser);
		result = node;
    }

    return result;
}

node_t *parse_expression(struct parser *parser)
{
	return parse_ternary_if(parser);
}

node_t *parse_low_boolean_unary(struct parser *parser)
{
    if(parser_current(parser) == T_NOT)
    {
    	node_t *result = alloc_node(parser, N_NOT);

		next(parser);

		result->left = parse_expression(parser);

		return result;
    }
    else
		return parse_expression(parser);
}

node_t *parse_low_boolean(struct parser *parser)
{
	node_t *result = parse_low_boolean_unary(parser);

    while(parser_current(parser) == T_AND || parser_current(parser) == T_OR)
    {
    	node_t *node = alloc_node(parser, N_BOOLEAN);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_low_boolean_unary(parser);
		result = node;
    }

    return result;
}

node_t *parse_statement(struct parser *parser)
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

node_t *parse_statements(struct parser *parser)
{
	skip_seps(parser);

	if(is_expression(parser))
	{
		node_t *result = parse_statement(parser);

		if (is_sep(parser))
		{
			skip_seps(parser);

			if(is_expression(parser))
			{
				node_t *node = alloc_node(parser, N_STATEMENTS);

				node->left = result;
				node->right = parse_statements(parser);

				return node;
			}
		}

		return result;

	}
	else
		return alloc_nil_node(parser);
}

node_t *parse_main(struct parser *parser)
{
	scope_t *scope;

	node_t *result = alloc_scope(parser, &scope, S_MAIN);

	scope->owner = scope;

	result->right = parse_statements(parser);

	match(parser, T_EOF);

	return result;
}

