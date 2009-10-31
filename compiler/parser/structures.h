#pragma once
#include "parser.h"

void parse_parameter(struct parser *parser, scope_t *scope);
node_t *parse_class(struct parser *parser);
node_t *parse_module(struct parser *parser);
node_t *parse_method(struct parser *parser);
