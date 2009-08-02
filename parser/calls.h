#include "parser.h"
#include "../runtime/classes.h"

struct node *parse_self_call(struct parser *parser, rt_value symbol);
struct node *parse_lookup_chain(struct parser *parser);
