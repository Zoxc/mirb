#include "compiler.h"
#include "ast.h"
#include "bytecode.h"
#include "block.h"
#include "../runtime/classes/string.h"
#include "../runtime/classes/symbol.h"

typedef rt_value (*get_node_name_proc)(struct node *node);

rt_value get_node_name(struct node *node);

rt_value name_unary_op(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, rt_string_from_cstr(token_type_names[node->op]));
	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_binary_op(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(" "));
	rt_concat_string(result, rt_string_from_cstr(token_type_names[node->op]));
	rt_concat_string(result, rt_string_from_cstr(" "));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_num(struct node *node)
{
	return rt_string_from_int((rt_value)node->left);
}

rt_value name_var(struct node *node)
{
	return rt_string_from_cstr(variable_name((rt_value)node->left));
}

rt_value name_const(struct node *node)
{
	rt_value result = get_node_name(node->left);

	rt_concat_string(result, rt_string_from_cstr("::"));
	rt_concat_string(result, rt_symbol_to_s((rt_value)node->right, 0, 0, 0));

	return result;
}

rt_value name_assign(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, rt_string_from_cstr(variable_name((rt_value)node->left)));
	rt_concat_string(result, rt_string_from_cstr(" = "));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_assign_const(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr("::"));
	rt_concat_string(result, rt_symbol_to_s((rt_value)node->middle, 0, 0, 0));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_if(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	if(node->type == N_UNLESS)
		rt_concat_string(result, rt_string_from_cstr("!"));

	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(" ? "));
	rt_concat_string(result, get_node_name(node->middle));
	rt_concat_string(result, rt_string_from_cstr(" : "));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_argument(struct node *node)
{
	if(node->right)
	{
		rt_value result = get_node_name(node->left);

		rt_concat_string(result, rt_string_from_cstr(", "));
		rt_concat_string(result, get_node_name(node->right));

		return result;
	}
	else
		return get_node_name(node->left);
}

rt_value name_call_tail(struct node *node)
{
	rt_value result = rt_string_from_cstr(token_type_names[node->op]);

	rt_concat_string(result, rt_string_from_cstr("("));
	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_call_arguments(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(")"));

	if(node->right)
	{
		rt_concat_string(result, rt_string_from_cstr("{"));
		rt_concat_string(result, get_node_name(node->right));
		rt_concat_string(result, rt_string_from_cstr("})"));
	}

	return result;
}

rt_value name_call(struct node *node)
{
	rt_value result = get_node_name(node->left);

	rt_concat_string(result, rt_string_from_cstr("."));
	rt_concat_string(result, rt_symbol_to_s((rt_value)node->middle, 0, 0, 0));
	rt_concat_string(result, get_node_name(node->right));

	return result;
}

rt_value name_array_call(struct node *node)
{
	rt_value result = get_node_name(node->left);

	rt_concat_string(result, rt_string_from_cstr(".[]"));

	if(node->middle)
	{
		rt_concat_string(result, rt_string_from_cstr("("));
		rt_concat_string(result, get_node_name(node->middle));
		rt_concat_string(result, rt_string_from_cstr(")"));
	}

	if(node->right)
	{
		rt_concat_string(result, rt_string_from_cstr("{"));
		rt_concat_string(result, get_node_name(node->right));
		rt_concat_string(result, rt_string_from_cstr("})"));
	}

	return result;
}

rt_value name_lookup_tail(struct node *node)
{
	return rt_string_from_cstr("(tail!");
}

rt_value name_expressions(struct node *node)
{
	rt_value result = get_node_name(node->left);

	if(node->right)
	{
		rt_concat_string(result, rt_string_from_cstr("; "));
		rt_concat_string(result, get_node_name(node->right));
	}

	return result;
}

rt_value name_class(struct node *node)
{
	rt_value result = rt_string_from_cstr("class ");

	rt_concat_string(result, rt_symbol_to_s((rt_value)node->left, 0, 0, 0));
	rt_concat_string(result, rt_string_from_cstr("("));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_module(struct node *node)
{
	rt_value result = rt_string_from_cstr("module ");

	rt_concat_string(result, rt_symbol_to_s((rt_value)node->left, 0, 0, 0));
	rt_concat_string(result, rt_string_from_cstr("("));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_scope(struct node *node)
{
	rt_value result = rt_string_from_cstr("scope:(");

	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_method(struct node *node)
{
	rt_value result = rt_string_from_cstr("def ");

	rt_concat_string(result, rt_symbol_to_s((rt_value)node->left, 0, 0, 0));
	rt_concat_string(result, rt_string_from_cstr("("));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_self(struct node *node)
{
	return rt_string_from_cstr("self");
}

rt_value name_true(struct node *node)
{
	return rt_string_from_cstr("true");
}

rt_value name_false(struct node *node)
{
	return rt_string_from_cstr("false");
}

rt_value name_nil(struct node *node)
{
	return rt_string_from_cstr("nil");
}

rt_value name_string(struct node *node)
{
	rt_value result = rt_string_from_cstr("\"");

	rt_concat_string(result, rt_string_from_cstr((const char *)node->left));
	rt_concat_string(result, rt_string_from_cstr("\""));

	return result;
}

rt_value name_string_continue(struct node *node)
{
	rt_value result;

	if(node->left)
	{
		result = get_node_name(node->left);

		rt_concat_string(result, rt_string_from_cstr("}"));
	}
	else
	{
		result = rt_string_from_cstr("\"");
	}

	rt_concat_string(result, rt_string_from_cstr((const char *)node->middle));
	rt_concat_string(result, rt_string_from_cstr("#{"));
	rt_concat_string(result, get_node_name(node->right));

	return result;
}

rt_value name_string_start(struct node *node)
{
	rt_value result = get_node_name(node->left);

	rt_concat_string(result, rt_string_from_cstr("}"));
	rt_concat_string(result, rt_string_from_cstr((const char *)node->right));

	return result;
}

rt_value name_array(struct node *node)
{
	rt_value result = rt_string_from_cstr("[");
	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr("]"));

	return result;
}

rt_value name_array_element(struct node *node)
{
	rt_value result = get_node_name(node->left);

	if(node->right)
	{
		rt_concat_string(result, rt_string_from_cstr(", "));
		rt_concat_string(result, get_node_name(node->right));
	}

	return result;
}

rt_value name_boolean(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");
	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(" "));
	rt_concat_string(result, rt_string_from_cstr(token_type_names[node->op]));
	rt_concat_string(result, rt_string_from_cstr(" "));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_not(struct node *node)
{
	rt_value result = rt_string_from_cstr("(not ");
	rt_concat_string(result, get_node_name(node->left));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_ivar(struct node *node)
{
	return rt_symbol_to_s((rt_value)node->left, 0, 0, 0);
}

rt_value name_ivar_assign(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, rt_symbol_to_s((rt_value)node->left, 0, 0, 0));
	rt_concat_string(result, rt_string_from_cstr(" = "));
	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_no_equality(struct node *node)
{
	rt_value result = rt_string_from_cstr("(");

	rt_concat_string(result, get_node_name(node->left));

	rt_concat_string(result, rt_string_from_cstr(" != "));

	rt_concat_string(result, get_node_name(node->right));
	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_handler(struct node *node)
{
	rt_value result = rt_string_from_cstr("(begin (");

	rt_concat_string(result, get_node_name(node->left));

	rt_concat_string(result, rt_string_from_cstr(")"));

	if(node->middle)
	{
		rt_concat_string(result, rt_string_from_cstr(" "));
		rt_concat_string(result, get_node_name(node->middle));
	}

	if(node->right)
	{
		rt_concat_string(result, rt_string_from_cstr(" ensure ("));

		rt_concat_string(result, get_node_name(node->right));

		rt_concat_string(result, rt_string_from_cstr(")"));
	}

	rt_concat_string(result, rt_string_from_cstr(")"));

	return result;
}

rt_value name_break_handler(struct node *node)
{
	rt_value result = rt_string_from_cstr("(begin (");

	rt_concat_string(result, get_node_name(node->left));

	rt_concat_string(result, rt_string_from_cstr(") catch break)"));

	return result;
}

rt_value name_rescue(struct node *node)
{
	rt_value result = rt_string_from_cstr("rescue (");

	rt_concat_string(result, get_node_name(node->left));

	rt_concat_string(result, rt_string_from_cstr(")"));

	if(node->right)
	{
		rt_concat_string(result, rt_string_from_cstr(" "));
		rt_concat_string(result, get_node_name(node->right));
	}

	return result;
}

rt_value name_return(struct node *node)
{
	rt_value result = rt_string_from_cstr("return ");

	rt_concat_string(result, get_node_name(node->left));

	return result;
}

rt_value name_break(struct node *node)
{
	rt_value result = rt_string_from_cstr("break ");

	rt_concat_string(result, get_node_name(node->left));

	return result;
}

rt_value name_next(struct node *node)
{
	rt_value result = rt_string_from_cstr("next ");

	rt_concat_string(result, get_node_name(node->left));

	return result;
}

rt_value name_redo(struct node *node)
{
	rt_value result = rt_string_from_cstr("redo ");

	rt_concat_string(result, get_node_name(node->left));

	return result;
}

get_node_name_proc get_node_name_procs[] = {
	name_unary_op,
	name_binary_op,
	name_num,
	name_var,
	name_ivar,
	name_ivar_assign,
	name_string,
	name_string_start,
	name_string_continue,
	name_array,
	name_array_element,
	name_const,
	name_self,
	name_true,
	name_false,
	name_nil,
	name_assign,
	name_assign_const,
	name_boolean,
	name_not,
	name_no_equality,
	name_if,
	name_if,
	name_return,
	name_next,
	name_redo,
	name_break,
	name_break_handler,
	name_handler,
	name_rescue,
	name_argument,
	name_call_arguments,
	name_call,
	name_array_call,
	name_expressions,
	name_class,
	name_module,
	name_scope,
	name_method
};

rt_value get_node_name(struct node *node)
{
    if (node)
        return get_node_name_procs[node->type](node);
    else
        return rt_string_from_cstr("");
}
