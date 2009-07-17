#include "parser.h"

inline bool match(token_type type)
{
    if(current_token.type == type)
    {
        next();

        return true;
    }
    else
    {
        printf("Excepted token %s but found %s\n", token_type_names[type], token_type_names[current_token.type]);

        return false;
    }
}

inline struct node *alloc_node(node_type type)
{
	struct node *result = malloc(sizeof(struct node));
	result->type = type;

	return result;
}

struct node *parse_factor(void)
{
	if(current_token.type == T_NUMBER)
	{
		struct node *result = alloc_node(N_FACTOR);

		result->left = get_token_str(&current_token);
		result->right = (void*)atoi(result->left);

		next();

		return result;
	}
	else
	{
		match(T_NUMBER);

		return 0;
	}
}

struct node *parse_unary(void)
{
    return parse_factor();
}

struct node *parse_multiplication(void)
{
	struct node *result = parse_unary();

    while(current_token.type == T_MUL || current_token.type == T_DIV)
    {
    	struct node *node = alloc_node(N_TERM);
		node->op = current_token.type;
		node->left = result;

		next();

		node->right = parse_unary();
		result = node;
    }

	return result;
}

struct node *parse_addition(void)
{
	struct node *result = parse_multiplication();

    while(current_token.type == T_ADD || current_token.type == T_SUB)
    {
    	struct node *node = alloc_node(N_EXPRESSION);
		node->op = current_token.type;
		node->left = result;

		next();

		node->right = parse_multiplication();
		result = node;
    }

    return result;
}

struct node *parse_expression(void)
{
    return parse_addition();
}
