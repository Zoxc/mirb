#pragma once
#include "parser.h"

void parse_parameter(struct compiler *compiler, struct block *block);
struct node *parse_class(struct compiler *compiler);
struct node *parse_module(struct compiler *compiler);
struct node *parse_method(struct compiler *compiler);
