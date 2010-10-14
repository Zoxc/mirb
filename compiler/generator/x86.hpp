#include "../../runtime/classes.hpp"
#include "../bytecode.hpp"
#include "../block.hpp"

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
 * %eax: rt_value *scopes[] - This is only used for closures.
 * %ecx: struct rt_class *module - The module where this methos was found.
 * %edx: struct rt_symbol *name - The name of the called method.
 *
 */

struct rt_block *compile_block(struct block *block);
