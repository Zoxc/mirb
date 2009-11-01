#pragma once
#include "parser.h"

void parse_parameter(struct compiler *compiler, scope_t *scope);
node_t *parse_class(struct compiler *compiler);
node_t *parse_module(struct compiler *compiler);
node_t *parse_method(struct compiler *compiler);
