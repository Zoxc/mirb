#pragma once
#include "parser.h"

struct node *parse_if(struct compiler *compiler);
struct node *parse_unless(struct compiler *compiler);
struct node *parse_ternary_if(struct compiler *compiler);
struct node *parse_conditional(struct compiler *compiler);
struct node *parse_case(struct compiler *compiler);
struct node *parse_begin(struct compiler *compiler);
struct node *parse_exception_handlers(struct compiler *compiler, struct node *block);
struct node *parse_return(struct compiler *compiler);
struct node *parse_break(struct compiler *compiler);
struct node *parse_next(struct compiler *compiler);
struct node *parse_redo(struct compiler *compiler);
