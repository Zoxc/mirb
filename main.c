#include "globals.h"
#include "lexer.h"
#include "parser.h"

typedef char*(*get_node_name_proc)(struct node *node);

char* get_node_name(struct node *node);

char* name_factor(struct node *node)
{
	return (char*)node->left;
}

char* name_arithmetics(struct node *node)
{
	char* left = get_node_name(node->left);
	char* right = get_node_name(node->right);
	char* result = malloc(100);

	sprintf(result, "(%s %s %s)", left, token_type_names[node->op], right);

	return result;
}

get_node_name_proc get_node_name_procs[] = {name_factor, name_arithmetics, name_arithmetics};

char* get_node_name(struct node *node)
{
	return get_node_name_procs[node->type](node);
}

typedef int(*executor)(struct node *node);

int exec_node(struct node *node);

int exec_factor(struct node *node)
{
	return (int)node->right;
}

int exec_arithmetics(struct node *node)
{
	int left = exec_node(node->left);
	int right = exec_node(node->right);
	int result;

	switch(node->op)
	{
		case T_MUL:
			result = left * right;
			break;

		case T_DIV:
			result = left / right;
			break;

		case T_ADD:
			result = left + right;
			break;

		case T_SUB:
			result = left - right;
			break;

		default:
			printf("Unknown op: %s", token_type_names[node->op]);
			return -1;
	}

	printf("%d.%s(%d) => %d\n", left, token_type_names[node->op], right, result);

	return result;
}

executor executors[] = {exec_factor, exec_arithmetics, exec_arithmetics};

int exec_node(struct node *node)
{
	return executors[node->type](node);
}

int main()
{
    setup_lexer();

    char buffer[800];
    gets(buffer);

    printf("Parsing: %s\n", input);

    lex(buffer);
    next();

    struct node* expression = parse_expression();

    printf("Tree: %s\n", get_node_name(expression));

    exec_node(expression);

    match(T_EOF);

    return 0;
}
