#include "parser.h"

struct node *parse_if(struct lexer* lexer);
struct node *parse_unless(struct lexer* lexer);
struct node *parse_ternary_if(struct lexer* lexer);
struct node *parse_conditional(struct lexer *lexer);
struct node *parse_case(struct lexer* lexer);
