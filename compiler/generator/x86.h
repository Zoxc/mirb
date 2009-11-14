#include "../../runtime/classes.h"
#include "../bytecode.h"
#include "../block.h"

/* Here is a description of the calling convention of the compiled blocks.
 *
 * The following arguments are passed on the stack, top to bottom:
 *
 * rt_value argv[] - A pointer to an array of arguments
 * size_t argc - The number of arguments
 * rt_value block - The attached block
 * rt_value obj - The self value
 *
 * The callee cleans up the stack.
 *
 * The following arguments are passed in registers:
 * struct rt_closure *closure - This is only used for closures. Passed in eax
 *
 */

rt_compiled_block_t compile_block(struct block *block);
