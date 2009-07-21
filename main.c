#include "globals.h"
#include "parser/lexer.h"
#include "parser/parser.h"
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

	if(node->type == T_UNLESS)
		sprintf(result, "(!%s ? %s : %s)", get_node_name(node->left), get_node_name(node->middle), get_node_name(node->right));
	else
		sprintf(result, "(%s ? %s : %s)", get_node_name(node->left), get_node_name(node->middle), get_node_name(node->right));

	return result;
}

char* name_nil(struct node *node)
{
	return "nil";
}

char* name_argument(struct node *node)
{
	char* result = malloc(100);

    sprintf(result, "%s, %s", get_node_name(node->left), get_node_name(node->right));

	return result;
}

char* name_message(struct node *node)
{
	char* result = malloc(100);

    if(node->middle)
    {
        if(node->right)
            sprintf(result, "%s(%s).%s", (char *)node->left, get_node_name(node->middle), get_node_name(node->right));
        else
            sprintf(result, "%s(%s)", (char *)node->left, get_node_name(node->middle));
    }
    else
    {
        if(node->right && node->right->type == N_MESSAGE)
            sprintf(result, "%s().%s", (char *)node->left, get_node_name(node->right));
		else if(node->right)
			sprintf(result, "%s%s", (char *)node->left, get_node_name(node->right));
        else
            sprintf(result, "%s()", (char *)node->left);
    }

	return result;
}

char* name_array_message(struct node *node)
{
	char* result = malloc(100);

	if (node->right)
		sprintf(result, "[]%s(%s, %s)", get_node_name(node->left), token_type_names[node->op - OP_TO_ASSIGN], get_node_name(node->right));
	else
		sprintf(result, "[](%s)", get_node_name(node->left));

	return result;
}

char* name_call_tail(struct node *node)
{
	char* result = malloc(100);

    sprintf(result, "%s(%s)", token_type_names[node->op], get_node_name(node->left));

	return result;
}

char* name_call(struct node *node)
{
	char* result = malloc(100);

	if (node->left)
		sprintf(result, "(%s.%s)", get_node_name(node->left), get_node_name(node->right));
	else
		sprintf(result, "(self.%s)", get_node_name(node->right));

	return result;
}

char* name_expressions(struct node *node)
{
	char* result = malloc(100);

	if(node->right)
		sprintf(result, "%s; %s", get_node_name(node->left), get_node_name(node->right));
	else
		return get_node_name(node->left);

	return result;
}

get_node_name_proc get_node_name_procs[] = {name_factor, name_factor, name_assign, name_arithmetics, name_arithmetics, name_if, name_if, name_nil, name_argument, name_message, name_array_message, name_call_tail, name_call, name_expressions};

char* get_node_name(struct node *node)
{
    if (node)
        return get_node_name_procs[node->type](node);
    else
        return "";
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

		struct node* expression = parse_expressions(lexer);
		match(lexer, T_EOF);
		lexer_destroy(lexer);

		if (lexer->err_count == 0)
		{
			printf("Parsing done.\n");
			printf("Tree: %s\n", get_node_name(expression));

			gen_block(expression);
		}
		else
			printf("Parsing failed.\n");
	}

	symbols_destroy();

	return 0;
}
