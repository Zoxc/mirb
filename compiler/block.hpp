#pragma once
#include "ast.hpp"
#include "bytecode.hpp"
#include "compiler.hpp"
#include "../runtime/runtime.hpp"
#include "../runtime/exceptions.hpp"

struct block;

/*
 * Block runtime data
 */

VEC_DEFAULT(struct exception_block *, rt_exception_blocks)

struct block_data {
	vec_t(rt_exception_blocks) exception_blocks;
	void **break_targets;
	size_t local_storage;
	void *epilog;
};

/*
 * Variables
 */

enum variable_type {
	V_HEAP,
	V_LOCAL,
	V_TEMP
};

#define VARIABLE_TYPES 3

struct temp_variable {
	enum variable_type type;
	size_t index;
};

struct variable {
	enum variable_type type;
	size_t index;
	struct block *owner;
	rt_value name;
};

HASH_COMPILER(block, size_t, struct variable *);
VEC_COMPILER(struct variable *, variables)
VEC_COMPILER(struct block *, blocks)
VEC_COMPILER(struct opcode *, opcodes)

/*
 * Block definition
 */

enum block_type {
	S_MAIN,
	S_METHOD,
	S_CLASS,
	S_MODULE,
	S_CLOSURE
};

struct block {
	enum block_type type;
	struct compiler *compiler; // The compiler which owns this block

	struct rt_block *output; // The runtime block structure this will all end in.

	struct block *parent; // The block enclosing this one
	struct block *owner; // The first parent that isn't a closure. This field can point to itself.

	/*
	 * Break related fields
	 */
	bool can_break; // If this block can raise a exception because of a break.
	size_t break_id; // Which of the parent break targets this block belongs to.
	size_t break_targets; // Number of child blocks that can raise a break exception.

	/*
	 * Exception related fields
	 */
	vec_t(rt_exception_blocks) exception_blocks;
	struct exception_block *current_exception_block;
	size_t current_exception_block_id;
	bool require_exceptions;

	struct block_data *data; // Runtime data structure

	/*
	 * Variable related fields
	 */
	rt_value var_count[VARIABLE_TYPES]; // An array of counters of the different variable types.
	hash_t(block) *variables; // A hash with all the variables declared or used
	vec_t(variables) parameters; // A list of all parameters except the block parameter.
	struct variable *block_parameter; // Pointer to a named or unnamed block variable.
	struct variable *super_module_var; // Pointer to a next module to search from if super is used.
	struct variable *super_name_var; // Pointer to a symbol which contains the name of the called method.
	vec_t(blocks) scopes; // A list of all the heap variable scopes this block requires.
	bool heap_vars; // If any of the variables must be stored on a heap scope.
	struct variable *scope_var; // A variable with the pointer to the heap scope
	struct variable *closure_var; // A variable with the pointer to the closure information
	vec_t(blocks) zsupers; // A list of all the blocks which requires access to all the parameters. Can only be used during parsing.

	#ifdef DEBUG
		rt_value label_count; // Nicer label labeling...
	#endif

	size_t self_ref;

	void *prolog; // The point after the prolog of the block.
	struct opcode *epilog; // The end of the block

	vec_t(opcodes) opcodes; // The bytecode output
};

/*
 * Block functions
 */

struct block *block_create(struct compiler *compiler, enum block_type type);

void block_require_var(struct block *current, struct variable *var);
void block_require_args(struct block *current, struct block *owner);

static inline void block_require_scope(struct block *block, struct block *scope)
{
	for(size_t i = 0; i < block->scopes.size; i++)
		if(block->scopes.array[i] == scope)
			return;

	vec_push(blocks, &block->scopes, scope);
}

static inline struct opcode *block_get_label(struct block *block)
{
	struct opcode *op = (struct opcode *)compiler_alloc(block->compiler, sizeof(struct opcode));
	op->type = B_LABEL;
	op->left = 0;

	#ifdef DEBUG
		op->right = block->label_count++;
	#endif

	return op;
}

static inline struct opcode *block_get_flush_label(struct block *block)
{
	struct opcode *op = block_get_label(block);
	op->left = L_FLUSH;

	return op;
}

static inline struct variable *block_get_var(struct block *block)
{
	struct variable *temp = (struct variable *)compiler_alloc(block->compiler, sizeof(struct temp_variable));
	temp->type = V_TEMP;
	temp->index = block->var_count[V_TEMP];

	block->var_count[V_TEMP] += 1;

	return temp;
}

static inline struct opcode *block_push(struct block *block, enum opcode_type type, rt_value result, rt_value left, rt_value right)
{
	struct opcode *op = (struct opcode *)compiler_alloc(block->compiler, sizeof(struct opcode));

	op->type = type;
	op->result = result;
	op->left = left;
	op->right = right;

	vec_push(opcodes, &block->opcodes, op);

	return op;
}

static inline struct opcode *block_emmit_label(struct block *block, struct opcode *label)
{
	vec_push(opcodes, &block->opcodes, label);

	return label;
}

static inline void block_print_label(rt_value label)
{
	#ifdef DEBUG
		printf("#%d", ((struct opcode *)label)->right);
	#else
		printf("#%x", label);
	#endif

}

const char *variable_name(rt_value var);

#ifdef DEBUG
	void block_print(struct block *block);
#endif