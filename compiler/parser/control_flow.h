#pragma once
#include "parser.h"

node_t *parse_if(struct compiler *compiler);
node_t *parse_unless(struct compiler *compiler);
node_t *parse_ternary_if(struct compiler *compiler);
node_t *parse_conditional(struct compiler *compiler);
node_t *parse_case(struct compiler *compiler);
node_t *parse_begin(struct compiler *compiler);
node_t *parse_exception_handlers(struct compiler *compiler, node_t *block);
node_t *parse_return(struct compiler *compiler);
