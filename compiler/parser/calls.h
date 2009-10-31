#pragma once
#include "parser.h"
#include "../../runtime/classes.h"

node_t *parse_call(struct parser *parser, rt_value symbol, node_t *child, bool default_var);
node_t *parse_lookup_chain(struct parser *parser);
node_t *parse_yield(struct parser *parser);
