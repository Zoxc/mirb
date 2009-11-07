#pragma once

#include "../bytecode.h"
#include "../block.h"
#include "../parser/parser.h"

struct block *gen_block(node_t* node);
