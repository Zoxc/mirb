#pragma once
#include "parser.h"
#include "../../runtime/classes.h"

node_t *parse_call(struct compiler *compiler, rt_value symbol, node_t *child, bool default_var);
node_t *parse_lookup_chain(struct compiler *compiler);
node_t *parse_yield(struct compiler *compiler);
