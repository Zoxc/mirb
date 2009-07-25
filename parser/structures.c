#include "structures.h"
#include "../runtime/symbols.h"

struct node *parse_class(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(N_CLASS);

	if(require(parser, T_IDENT))
	{
		result->left = (void *)symbol_from_parser(parser);

		next(parser);
	}
	else
		result->left = 0;

	parse_sep(parser);

	result->right = alloc_scope(parser, N_SCOPE);
	result->right->right = parse_statements(parser);

	match(parser, T_END);

	return result;
}
