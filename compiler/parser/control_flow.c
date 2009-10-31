#include "control_flow.h"

struct node *parse_return(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(parser, N_RETURN);

	if(is_expression(parser))
		result->left = parse_expression(parser);
	else
		result->left = 0;

	return result;
}

struct node *parse_exception_handlers(struct parser *parser, struct node *block)
{
	struct node *parent = alloc_node(parser, N_HANDLER);

	parent->left = block;

	if(parser_current(parser) == T_RESCUE)
	{
		next(parser);

		struct node *rescue = alloc_node(parser, N_RESCUE);
		rescue->left = parse_statements(parser);
		rescue->right = 0;

		parent->middle = rescue;

		while(parser_current(parser) == T_RESCUE)
		{
			next(parser);

			struct node *node = alloc_node(parser, N_RESCUE);
			node->left = parse_statements(parser);
			node->right = 0;

			rescue->right = node;
			rescue = node;
		}
	}
	else
		parent->middle = 0;

	if(parser_current(parser) == T_ENSURE)
	{
		next(parser);

		parent->right = parse_statements(parser);
	}
	else
		parent->right = 0;

	return parent;
}

struct node *parse_begin(struct parser *parser)
{
	next(parser);

	struct node *result = parse_statements(parser);

	switch (parser_current(parser))
	{
		case T_ENSURE:
		case T_RESCUE:
			result = parse_exception_handlers(parser, result);
			break;

		default:
			break;
	}

	match(parser, T_END);

	return result;
}

void parse_then_sep(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_THEN:
		case T_COLON:
			next(parser);
			break;

		default:
			parse_sep(parser);
	}
}

struct node *parse_ternary_if(struct parser *parser)
{
	struct node *result = parse_boolean_or(parser);

	if(parser_current(parser) == T_QUESTION)
	{
		next(parser);

    	struct node *node = alloc_node(parser, N_IF);

		node->left = result;
		node->middle = parse_ternary_if(parser);

		match(parser, T_COLON);

		node->right = parse_ternary_if(parser);

		return node;
	}

	return result;
}

struct node *parse_conditional(struct parser *parser)
{
	struct node *result = parse_low_boolean(parser);

    if (parser_current(parser) == T_IF || parser_current(parser) == T_UNLESS)
    {
     	struct node *node = alloc_node(parser, parser_current(parser) == T_IF ? N_IF : N_UNLESS);

		next(parser);

		node->middle = result;
		node->left = parse_statement(parser);
		node->right = alloc_nil_node(parser);

		return node;
    }

    return result;
}

struct node *parse_unless(struct parser *parser)
{
				next(parser);

				struct node *result = alloc_node(parser, N_UNLESS);
				result->left = parse_expression(parser);

				parse_then_sep(parser);

				result->middle = parse_statements(parser);
				result->right = alloc_nil_node(parser);

				match(parser, T_END);

				return result;
}

struct node *parse_if_tail(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_ELSIF:
			{
				next(parser);

				struct node *result = alloc_node(parser, N_IF);
				result->left = parse_expression(parser);

				parse_then_sep(parser);

				result->middle = parse_statements(parser);
				result->right = parse_if_tail(parser);

				return result;
			}

		case T_ELSE:
			next(parser);

			return parse_statements(parser);

		default:
			return alloc_nil_node(parser);
	}
}

struct node *parse_if(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(parser, N_IF);
	result->left = parse_expression(parser);

	parse_then_sep(parser);

	result->middle = parse_statements(parser);
	result->right = parse_if_tail(parser);

	match(parser, T_END);

	return result;
}

struct node *parse_case_body(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_WHEN:
			{
				next(parser);

				struct node *result = alloc_node(parser, N_IF);
				result->left = parse_expression(parser);

				parse_then_sep(parser);

				result->middle = parse_statements(parser);
				result->right = parse_case_body(parser);

				return result;
			}

		case T_ELSE:
			{
				next(parser);

				return parse_statements(parser);
			}

		default:
			{
				PARSER_ERROR(parser, "Expected else or when but found %s", token_type_names[parser_current(parser)]);

				return 0;
			}
	}
}

struct node *parse_case(struct parser *parser)
{
	next(parser);

	struct node *result = 0;

	switch (parser_current(parser))
	{
		case T_ELSE:
			next(parser);

			result = parse_statements(parser);
			break;

		case T_WHEN:
			result = parse_case_body(parser);
			break;

		default:
			PARSER_ERROR(parser, "Expected else or when but found %s", token_type_names[parser_current(parser)]);
	}

	match(parser, T_END);

	return result;
}
