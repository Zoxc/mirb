#include "parser.h"

void parse_parameter(struct parser *parser, scope_t *scope);
struct node *parse_class(struct parser *parser);
struct node *parse_module(struct parser *parser);
struct node *parse_method(struct parser *parser);
