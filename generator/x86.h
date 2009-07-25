#include "../runtime/classes.h"
#include "bytecode.h"
typedef rt_value(*compiled_block_t)();

compiled_block_t compile_block(block_t *block);
