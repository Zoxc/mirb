#include "parser.h"
#include "../runtime/classes.h"

struct node *parse_call(struct parser *parser, rt_value symbol, struct node *child, bool default_var);
struct node *parse_lookup_chain(struct parser *parser);
struct node *parse_yield(struct parser *parser);
