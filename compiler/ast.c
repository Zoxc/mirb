#include "compiler.h"
#include "ast.h"
#include "bytecode.h"
#include "block.h"
#include "../runtime/classes/string.h"
#include "../runtime/classes/symbol.h"

typedef void (*parent_proc)(struct node *node, struct node *parent);

void parent_unary_op(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_binary_op(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_num(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_var(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_const(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_assign(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_assign_const(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_if(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->middle, node);
	parent_node(node->right, node);
}

void parent_argument(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_call_tail(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_call_arguments(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_call(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_array_call(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->middle, node);
	parent_node(node->right, node);
}

void parent_expressions(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_class(struct node *node, struct node *parent)
{
	parent_node(node->right, node);
}

void parent_module(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_scope(struct node *node, struct node *parent)
{
	parent_node(node->right, node);
}

void parent_method(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_self(struct node *node, struct node *parent)
{
}

void parent_true(struct node *node, struct node *parent)
{
}

void parent_false(struct node *node, struct node *parent)
{
}

void parent_nil(struct node *node, struct node *parent)
{
}

void parent_string(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_string_continue(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_string_start(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_array(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_array_element(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_boolean(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_not(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_ivar(struct node *node, struct node *parent)
{
}

void parent_ivar_assign(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_no_equality(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_handler(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->middle, node);
	parent_node(node->right, node);
}

void parent_break_handler(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_rescue(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_return(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_break(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_next(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
}

void parent_redo(struct node *node, struct node *parent)
{
}

void parent_super(struct node *node, struct node *parent)
{
	parent_node(node->left, node);
	parent_node(node->right, node);
}

void parent_zsuper(struct node *node, struct node *parent)
{
	parent_node(node->right, node);
}

parent_proc parent_node_procs[] = {
	parent_unary_op,
	parent_binary_op,
	parent_num,
	parent_var,
	parent_ivar,
	parent_ivar_assign,
	parent_string,
	parent_string_start,
	parent_string_continue,
	parent_array,
	parent_array_element,
	parent_const,
	parent_self,
	parent_true,
	parent_false,
	parent_nil,
	parent_assign,
	parent_assign_const,
	parent_boolean,
	parent_not,
	parent_no_equality,
	parent_if,
	parent_if,
	parent_super,
	parent_zsuper,
	parent_return,
	parent_next,
	parent_redo,
	parent_break,
	parent_break_handler,
	parent_handler,
	parent_rescue,
	parent_argument,
	parent_call_arguments,
	parent_call,
	parent_array_call,
	parent_expressions,
	parent_class,
	parent_module,
	parent_scope,
	parent_method
};

void parent_node(struct node *node, struct node *parent)
{
	if(node)
	{
		node->parent = parent;

		parent_node_procs[node->type](node, parent);
	}
}

#ifdef DEBUG
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
		rt_concat_string(result, rt_string_from_symbol((rt_value)node->right));

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
		rt_concat_string(result, rt_string_from_symbol((rt_value)node->middle));
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
		rt_concat_string(result, rt_string_from_symbol((rt_value)node->middle));
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

		rt_concat_string(result, rt_string_from_symbol((rt_value)node->left));
		rt_concat_string(result, rt_string_from_cstr("("));
		rt_concat_string(result, get_node_name(node->right));
		rt_concat_string(result, rt_string_from_cstr(")"));

		return result;
	}

	rt_value name_module(struct node *node)
	{
		rt_value result = rt_string_from_cstr("module ");

		rt_concat_string(result, rt_string_from_symbol((rt_value)node->left));
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

		rt_concat_string(result, rt_string_from_symbol((rt_value)node->left));
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

			rt_concat_string(result, rt_string_from_cstr("}\""));
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
		return rt_string_from_symbol((rt_value)node->left);
	}

	rt_value name_ivar_assign(struct node *node)
	{
		rt_value result = rt_string_from_cstr("(");

		rt_concat_string(result, rt_string_from_symbol((rt_value)node->left));
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
		rt_value result = rt_string_from_cstr("redo");

		return result;
	}

	rt_value name_super(struct node *node)
	{
		rt_value result = rt_string_from_cstr("super(");

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

	rt_value name_zsuper(struct node *node)
	{
		rt_value result = rt_string_from_cstr("zsuper");

		if(node->right)
		{
			rt_concat_string(result, rt_string_from_cstr(" {"));
			rt_concat_string(result, get_node_name(node->right));
			rt_concat_string(result, rt_string_from_cstr("})"));
		}

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
		name_super,
		name_zsuper,
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
		if(node)
			return get_node_name_procs[node->type](node);
		else
			return rt_string_from_cstr("");
	}
#endif
