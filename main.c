#include "globals.h"
#include "lexer.h"
#include "parser.h"
#include "symbols.h"
#include "generator.h"

KHASH_MAP_INIT_INT(var, int);

static khash_t(var) *var_list;

typedef char*(*get_node_name_proc)(struct node *node);

char* get_node_name(struct node *node);

char* name_factor(struct node *node)
{
	return (char*)node->left;
}

char* name_assign(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "(%s = %s)", (char *)node->left, get_node_name(node->right));

	return result;
}

char* name_arithmetics(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "(%s %s %s)", get_node_name(node->left), token_type_names[node->op], get_node_name(node->right));

	return result;
}

char* name_if(struct node *node)
{
	char* result = malloc(100);

	if(node->right->type == N_ELSE)
		sprintf(result, "(%s ? %s : %s)", get_node_name(node->left), get_node_name(node->right->left), get_node_name(node->right->right));
	else
		sprintf(result, "(%s ? %s)", get_node_name(node->left), get_node_name(node->right));

	return result;
}

get_node_name_proc get_node_name_procs[] = {name_factor, name_factor, name_assign, name_arithmetics, name_arithmetics, name_if, 0/*N_ELSE*/};

char* get_node_name(struct node *node)
{
	return get_node_name_procs[node->type](node);
}

typedef int(*executor)(struct node *node);

int exec_node(struct node *node);

int exec_num(struct node *node)
{
	return (int)node->right;
}

int exec_var(struct node *node)
{
	khiter_t k = kh_get(var, var_list, (int)node->left);

	if (k == kh_end(var_list))
	{
		printf("Unknown variable %s => 0\n", (char *)node->left);

		return 0;
	}
	else
	{
		int result = kh_value(var_list, k);

		printf("Variable %s => %d\n", (char *)node->left, result);

		return result;
	}
}

int exec_assign(struct node *node)
{
	int result = exec_node(node->right);

	int ret;

	khiter_t k = kh_get(var, var_list, (int)node->left);

	if (k == kh_end(var_list))
	{
		k = kh_put(var, var_list, (int)node->left, &ret);

		if(!ret)
		{
			kh_del(var, var_list, k);

			printf("Unable to store value %d to variable %s\n", result, (char *)node->left);
		}
	}

	kh_value(var_list, k) = result;

	printf("%s = %d => %d\n", (char *)node->left, result, result);

	return result;
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

int exec_if(struct node *node)
{
	int left = exec_node(node->left);
	int result;

	if(node->right->type == N_ELSE)
	{
		if(left)
		{
			result = exec_node(node->right->left);
			printf("%d ? <%d> : %s  => %d\n", left, result, get_node_name(node->right->right), result);
		}
		else
		{
			result = exec_node(node->right->right);
			printf("%d ? %s : <%d> => %d\n", left, get_node_name(node->right->left), result, result);
		}
	}
	else
	{
		if(left)
		{
			result = exec_node(node->right);
			printf("%d ? <%d> => %d\n", left, result, result);
		}
		else
		{
			result = -1;
			printf("%d ? %s => %d\n", left, get_node_name(node->right), result);
		}
	}

	return result;
}

executor executors[] = {exec_num, exec_var, exec_assign, exec_arithmetics, exec_arithmetics, exec_if, 0/*N_ELSE*/};

int exec_node(struct node *node)
{
	return executors[node->type](node);
}

int main()
{
	char buffer[800];

	lexer_setup();
	symbols_create();

	var_list = kh_init(var);

	while(1)
	{
		gets(buffer);

		struct lexer *lexer = lexer_create(buffer);

		if(lexer_current(lexer) == T_EOF)
		{
			lexer_destroy(lexer);

			break;
		}

		struct node* expression = parse_expression(lexer);
		match(lexer, T_EOF);
		printf("parser done.");
		lexer_destroy(lexer);

		if (lexer->err_count == 0)
		{
			printf("Tree: %s\n", get_node_name(expression));

			exec_node(expression);
			gen_block(expression);
		}
	}

	symbols_destroy();

	return 0;
}
