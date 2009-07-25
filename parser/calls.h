#include "parser.h"
#include "../runtime/classes.h"

struct node *parse_message(struct parser *parser, struct node **tail, rt_value symbol);
void parse_call_tail(struct parser *parser, struct node *tail);
struct node *parse_call(struct parser *parser, struct node *object);

