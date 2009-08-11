#include "globals.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "runtime/symbol.h"
#include "runtime/classes.h"
#include "generator/generator.h"
#include "generator/x86.h"

typedef char*(*get_node_name_proc)(struct node *node);

char* get_node_name(struct node *node);

char* name_num(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "%d", node->left);

	return result;
}

char* name_var(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "%%%d", (rt_value)node->left);

	return result;
}

char* name_const(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "%s::%s", get_node_name(node->left), rt_symbol_to_cstr((rt_value)node->right));

	return result;
}

char* name_assign(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "(%%%d = %s)", (rt_value)node->left, get_node_name(node->right));

	return result;
}

char* name_assign_const(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "(%s::%s = %s)", get_node_name(node->left), rt_symbol_to_cstr((rt_value)node->middle), get_node_name(node->right));

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

	if(node->type == N_UNLESS)
		sprintf(result, "(!%s ? %s : %s)", get_node_name(node->left), get_node_name(node->middle), get_node_name(node->right));
	else
		sprintf(result, "(%s ? %s : %s)", get_node_name(node->left), get_node_name(node->middle), get_node_name(node->right));

	return result;
}

char* name_argument(struct node *node)
{
	char* result = malloc(100);

	if(node->right)
		sprintf(result, "%s, %s", get_node_name(node->left), get_node_name(node->right));
	else
		return get_node_name(node->left);

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

	if((rt_value)node->right > 1)
		sprintf(result, "%s.%s(%s)", get_node_name(node->left), rt_symbol_to_cstr((rt_value)node->middle), get_node_name(node->right));
	else
		sprintf(result, "%s.%s()", get_node_name(node->left), rt_symbol_to_cstr((rt_value)node->middle));

	return result;
}

char* name_assign_call(struct node *node)
{
	char* result = malloc(100);

	if((rt_value)node->right > 1)
		sprintf(result, "%s.%s(%s)", get_node_name(node->left), rt_symbol_to_cstr((rt_value)node->middle), get_node_name(node->right));
	else
		sprintf(result, "%s.%s()", get_node_name(node->left), rt_symbol_to_cstr((rt_value)node->middle));

	return result;
}

char* name_lookup_tail(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "tail!");

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

char* name_class(struct node *node)
{
	char* result = malloc(100);

	if(node->right)
		sprintf(result, "class %s(%s)", rt_symbol_to_cstr((rt_value)node->left), get_node_name(node->right));
	else
		return get_node_name(node->left);

	return result;
}

char* name_scope(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "scope:(%s)", get_node_name(node->right));

	return result;
}

char* name_method(struct node *node)
{
	char* result = malloc(100);

	if(node->right)
		sprintf(result, "def %s(%s)", rt_symbol_to_cstr((rt_value)node->left), get_node_name(node->right));
	else
		return get_node_name(node->left);

	return result;
}

char* name_self(struct node *node)
{
	return "self";
}

char* name_true(struct node *node)
{
	return "true";
}

char* name_false(struct node *node)
{
	return "false";
}

char* name_nil(struct node *node)
{
	return "nil";
}

char* name_string(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "\"%s\"", node->left);

	return result;
}

char* name_string_continue(struct node *node)
{
	char* result = malloc(100);

	if(node->left)
		sprintf(result, "%s}%s#{%s", get_node_name(node->left), node->middle, get_node_name(node->right));
	else
		sprintf(result, "\"%s#{%s", node->middle, get_node_name(node->right));

	return result;
}

char* name_string_start(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "%s}%s\"", get_node_name(node->left), node->right);

	return result;
}

char* name_unary(struct node *node)
{
	char* result = malloc(100);

	sprintf(result, "(%s%s)", token_type_names[node->op], get_node_name(node->left));

	return result;
}


get_node_name_proc get_node_name_procs[] = {name_num, name_var, name_string, name_string_start, name_string_continue, name_const, name_self, name_true, name_false, name_nil, name_assign, name_assign_const, name_unary, name_arithmetics, name_arithmetics, name_if, name_if, name_argument, name_call, name_assign_call, name_expressions, name_class, name_scope, name_method};

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

	parser_setup();

	rt_create();

	while(1)
	{
		gets(buffer);

		struct parser *parser = parser_create(buffer);

		if(parser_current(parser) == T_EOF)
		{
			parser_destroy(parser);

			break;
		}

		struct node* expression = parse_main(parser);

		if (parser->err_count == 0)
		{
			printf("Parsing done.\n");
			printf("Tree: %s\n", get_node_name(expression));

			block_t *block = gen_block(expression);

			rt_compiled_block_t compiled_block = compile_block(block);

			printf("Running block %x:\n", compiled_block);

			rt_value result = compiled_block(rt_main, 1);

			printf(" => "); rt_print(result); printf("\n");
		}
		else
			printf("Parsing failed.\n");

		parser_destroy(parser);
	}

	rt_destroy();

	return 0;
}
