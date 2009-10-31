#pragma once
#include "parser.h"

node_t *parse_if(struct parser *parser);
node_t *parse_unless(struct parser *parser);
node_t *parse_ternary_if(struct parser *parser);
node_t *parse_conditional(struct parser *parser);
node_t *parse_case(struct parser *parser);
node_t *parse_begin(struct parser *parser);
node_t *parse_exception_handlers(struct parser *parser, node_t *block);
node_t *parse_return(struct parser *parser);
