#include "parser.h"

struct node *parse_message(struct lexer* lexer, struct node **tail, void *symbol);
void parse_call_tail(struct lexer* lexer, struct node *tail);
struct node *parse_call(struct lexer* lexer, struct node *object);

