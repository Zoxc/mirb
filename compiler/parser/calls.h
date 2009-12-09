#pragma once
#include "parser.h"
#include "../../runtime/classes.h"

struct node *parse_call(struct compiler *compiler, rt_value symbol, struct node *child, bool default_var);
struct node *parse_lookup_chain(struct compiler *compiler);
struct node *parse_yield(struct compiler *compiler);
struct node *parse_super(struct compiler *compiler);
