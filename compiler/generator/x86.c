#include "x86.h"
#include "../../runtime/x86.h"
#include "../../runtime/support.h"
#include "../../runtime/code_heap.h"
#include "../../runtime/method.h"
#include "../../runtime/constant.h"
#include "disassembly.h"

static inline size_t label_target(void *target, rt_value label)
{
	struct opcode *op = (struct opcode *)label;

	return ((size_t)target + op->result);
}

#define GEN_BYTE(output) do \
	{ \
		if(!measuring) \
			**(uint8_t**)target = (uint8_t)(output); \
		\
		*target += sizeof(uint8_t);\
	} \
	while(0)

#define GEN_WORD(output) do \
	{ \
		if(!measuring) \
			**(uint16_t**)target = (uint16_t)(output); \
		\
		*target += sizeof(uint16_t);\
	} \
	while(0)

#define GEN_DWORD(output) do \
	{ \
		if(!measuring) \
			**(uint32_t**)target = (uint32_t)(output); \
		\
		*target += sizeof(uint32_t);\
	} \
	while(0)

static inline int get_stack_index(struct block *block, rt_value var)
{
	struct variable *_var = (struct variable *)var;

	int index;

	switch(_var->type)
	{
		case V_TEMP:
			index = (block->require_exceptions ? 5 : 0) + block->var_count[V_LOCAL] + _var->index;
			break;

		case V_LOCAL:
			index = (block->require_exceptions ? 5 : 0) + _var->index;
			break;

		default:
			RT_ASSERT(0);
	}

	return -((index + 1) * 4);
}

static inline void generate_call(uint8_t **target, void *function, bool measuring)
{
	GEN_BYTE(0xE8);
	GEN_DWORD((size_t)function - ((size_t)*target - 1) - 5);
}

static inline void generate_stack_push(uint8_t **target, size_t dword, bool measuring)
{
	GEN_BYTE(0x68);
	GEN_DWORD(dword);
}

static inline size_t get_scope_index(struct block *block, struct block *scope)
{
	for(size_t i = 0; i < kv_size(block->scopes); i++)
		if(kv_A(block->scopes, i) == scope)
			return i;

	RT_ASSERT(0);
}

static inline void generate_stack_var_push(struct block *block, uint8_t **target, rt_value var, bool measuring)
{
	struct variable * _var = (struct variable *)var;

	switch(_var->type)
	{
		case V_LOCAL:
		case V_TEMP:
			GEN_BYTE(0xFF);
			GEN_BYTE(0x75);
			GEN_BYTE((char)get_stack_index(block, var));
			break;

		case V_HEAP:
			if(_var->owner == block)
			{
				/*
				 * Push the real variable from ebx
				 */
				GEN_BYTE(0xFF); // push dword [ebx + _var->index]
				GEN_BYTE(0x73);
				GEN_BYTE((char)_var->index * 4);
			}
			else
			{
				/*
				 * Load the scope pointer to ecx
				 */
				GEN_BYTE(0x8B); // mov ecx, dword [esi + get_scope_index(block, _var->owner)]
				GEN_BYTE(0x4E);
				GEN_BYTE((char)get_scope_index(block, _var->owner));

				/*
				 * Push the real variable from ecx
				 */
				GEN_BYTE(0xFF); // push dword [ecx + _var->index]
				GEN_BYTE(0x71);
				GEN_BYTE((char)_var->index * 4);
			}
			break;

		default:
			RT_ASSERT(0);
	}
}

static inline void generate_var_load(struct block *block, uint8_t **target, rt_value var, bool measuring)
{
	struct variable * _var = (struct variable *)var;

	switch(_var->type)
	{
		case V_LOCAL:
		case V_TEMP:
			GEN_BYTE(0x8B);
			GEN_BYTE(0x45);
			GEN_BYTE((char)get_stack_index(block, var));
			break;

		case V_HEAP:
			if(_var->owner == block)
			{
				/*
				 * Load the real variable to eax
				 */
				GEN_BYTE(0x8B); // mov eax, dword [ebx + _var->index]
				GEN_BYTE(0x43);
				GEN_BYTE((char)_var->index * 4);
			}
			else
			{
				/*
				 * Load the scope pointer to eax
				 */
				GEN_BYTE(0x8B); // mov eax, dword [esi + get_scope_index(block, _var->owner)]
				GEN_BYTE(0x46);
				GEN_BYTE((char)get_scope_index(block, _var->owner));

				/*
				 * Load the real variable to eax
				 */
				GEN_BYTE(0x8B); // mov eax, dword [eax + _var->index]
				GEN_BYTE(0x40);
				GEN_BYTE((char)_var->index * 4);
			}
			break;

		default:
			RT_ASSERT(0);
	}
}

static inline void generate_var_store(struct block *block, uint8_t **target, rt_value var, bool measuring)
{
	struct variable * _var = (struct variable *)var;

	switch(_var->type)
	{
		case V_LOCAL:
		case V_TEMP:
			GEN_BYTE(0x89); // mov dword [ebp + get_stack_index(block, var)], eax
			GEN_BYTE(0x45);
			GEN_BYTE((char)get_stack_index(block, var));
			break;

		case V_HEAP:
			if(_var->owner == block)
			{
				/*
				 * Store eax in the real variable
				 */
				GEN_BYTE(0x89); // mov dword [ebx + _var->index], eax
				GEN_BYTE(0x43);
				GEN_BYTE((char)_var->index * 4);
			}
			else
			{
				/*
				 * Load the scope pointer to ecx
				 */
				GEN_BYTE(0x8B); // mov ecx, dword [esi + get_scope_index(block, _var->owner)]
				GEN_BYTE(0x4E);
				GEN_BYTE((char)get_scope_index(block, _var->owner));

				/*
				 * Store eax in the real variable
				 */
				GEN_BYTE(0x89); // mov dword [ecx + _var->index], eax
				GEN_BYTE(0x41);
				GEN_BYTE((char)_var->index * 4);
			}
			break;

		default:
			RT_ASSERT(0);
	}
}

static inline void generate_var_store_imm(struct block *block, uint8_t **target, rt_value var, rt_value value, bool measuring)
{
	struct variable * _var = (struct variable *)var;

	switch(_var->type)
	{
		case V_LOCAL:
		case V_TEMP:
			GEN_BYTE(0xC7); // mov dword [ebp + get_stack_index(block, var)], value
			GEN_BYTE(0x45);
			GEN_BYTE((char)get_stack_index(block, var));
			GEN_DWORD(value);
			break;

		case V_HEAP:
			if(_var->owner == block)
			{
				/*
				 * Store value in the real variable
				 */
				GEN_BYTE(0xC7); // mov dword [ebx + _var->index], value
				GEN_BYTE(0x43);
				GEN_BYTE((char)_var->index * 4);
				GEN_DWORD(value);
			}
			else
			{
				/*
				 * Load the scope pointer to ecx
				 */
				GEN_BYTE(0x8B); // mov ecx, dword [esi + get_scope_index(block, _var->owner)]
				GEN_BYTE(0x4E);
				GEN_BYTE((char)get_scope_index(block, _var->owner));

				/*
				 * Store value in the real variable
				 */
				GEN_BYTE(0xC7); // mov dword [ecx + _var->index], value
				GEN_BYTE(0x41);
				GEN_BYTE((char)_var->index * 4);
				GEN_DWORD(value);
			}
			break;

		default:
			RT_ASSERT(0);
	}
}

static inline void generate_var_store_self(struct block *block, uint8_t **target, rt_value var, bool measuring)
{
	struct variable * _var = (struct variable *)var;

	switch(_var->type)
	{
		case V_LOCAL:
		case V_TEMP:
			GEN_BYTE(0x89); // mov dword [ebp + get_stack_index(block, var)], eax
			GEN_BYTE(0x7D);
			GEN_BYTE((char)get_stack_index(block, var));
			break;

		case V_HEAP:
			if(_var->owner == block)
			{
				/*
				 * Store eax in the real variable
				 */
				GEN_BYTE(0x89); // mov dword [ebx + _var->index], eax
				GEN_BYTE(0x7B);
				GEN_BYTE((char)_var->index * 4);
			}
			else
			{
				/*
				 * Load the scope pointer to ecx
				 */
				GEN_BYTE(0x8B); // mov ecx, dword [esi + get_scope_index(block, _var->owner)]
				GEN_BYTE(0x4E);
				GEN_BYTE((char)get_scope_index(block, _var->owner));

				/*
				 * Store eax in the real variable
				 */
				GEN_BYTE(0x89); // mov dword [ecx + _var->index], eax
				GEN_BYTE(0x79);
				GEN_BYTE((char)_var->index * 4);
			}
			break;

		default:
			RT_ASSERT(0);
	}
}

static inline void generate_var_cmp(struct block *block, uint8_t **target, rt_value var, bool measuring)
{
	struct variable * _var = (struct variable *)var;

	switch(_var->type)
	{
		case V_LOCAL:
		case V_TEMP:
			GEN_BYTE(0x3B); // cmp eax, dword [ebp + get_stack_index(block, var)]
			GEN_BYTE(0x45);
			GEN_BYTE((char)get_stack_index(block, var));
			break;

		case V_HEAP:
			if(_var->owner == block)
			{
				/*
				 * Compare eax with the real variable
				 */
				GEN_BYTE(0x3B); // cmp eax, dword [ebx + _var->index]
				GEN_BYTE(0x43);
				GEN_BYTE((char)_var->index * 4);
			}
			else
			{
				/*
				 * Load the scope pointer to ecx
				 */
				GEN_BYTE(0x8B); // mov ecx, dword [esi + get_scope_index(block, _var->owner)]
				GEN_BYTE(0x4E);
				GEN_BYTE((char)get_scope_index(block, _var->owner));

				/*
				 * Compare eax with the real variable
				 */
				GEN_BYTE(0x3B); // cmp eax, dword [ecx + _var->index]
				GEN_BYTE(0x41);
				GEN_BYTE((char)_var->index * 4);
			}
			break;

		default:
			RT_ASSERT(0);
	}
}

static inline void generate_stack_pop(uint8_t **target, size_t bytes, bool measuring)
{
	GEN_BYTE(0x81);
	GEN_BYTE(0xC4);
	GEN_DWORD(bytes);
}

static inline void generate_instruction(struct block *block, struct opcode *op, size_t i, uint8_t *start, uint8_t **target, bool measuring)
{
	switch(op->type)
	{
		case B_SEAL:
			{
				//TODO: Support for block sealing
			}
			break;

		case B_PUSH_SCOPE:
			{
				if((struct block *)op->result == block)
					GEN_BYTE(0x53); // push ebx
				else
				{
					GEN_BYTE(0xFF); // push dword [esi + get_scope_index(block, (struct block *)op->result)]
					GEN_BYTE(0x76);
					GEN_BYTE((char)get_scope_index(block, (struct block *)op->result));
				}
			}
			break;

		case B_CLOSURE:
			{
				if(block->super_module_var)
				{
					generate_stack_var_push(block, target, (rt_value)block->super_module_var, measuring);
					generate_stack_var_push(block, target, (rt_value)block->super_name_var, measuring);
				}
				else
				{
					generate_stack_push(target, RT_NIL, measuring);
					generate_stack_push(target, RT_NIL, measuring);
				}

				generate_stack_push(target, measuring ? 0 : (size_t)compile_block((struct block *)op->left), measuring);

				generate_call(target, rt_support_closure, measuring);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 5) * 4, measuring);

				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_CLASS:
			{
				struct block *class_block = (struct block *)op->right;
				rt_compiled_block_t compiled = measuring ? 0 : compile_block(class_block);

				if(op->left)
					generate_stack_var_push(block, target, op->left, measuring);
				else
					generate_stack_push(target, rt_Object, measuring);

				generate_stack_push(target, op->result, measuring);
				generate_call(target, rt_support_define_class, measuring);

				GEN_BYTE(0x50); // push dummy argv
				generate_stack_push(target, 0, measuring); // push argc
				generate_stack_push(target, 0, measuring); // push block
				GEN_BYTE(0x50); // push obj(eax)

				generate_call(target, compiled, measuring);
			}
			break;

		case B_MODULE:
			{
				struct block *module_block = (struct block *)op->left;
				rt_compiled_block_t compiled = measuring ? 0 : compile_block(module_block);

				generate_stack_push(target, op->result, measuring);
				generate_call(target, rt_support_define_module, measuring);

				GEN_BYTE(0x50); // push dummy argv
				generate_stack_push(target, 0, measuring); // push argc
				generate_stack_push(target, 0, measuring); // push block
				GEN_BYTE(0x50); // push obj(eax)

				generate_call(target, compiled, measuring);
			}
			break;

		case B_GET_IVAR:
			{
				GEN_BYTE(0xB8); // mov eax,
				GEN_DWORD(op->left);

				generate_call(target, rt_support_get_ivar, measuring);

				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_SET_IVAR:
			{
				GEN_BYTE(0xB8); // mov eax,
				GEN_DWORD(op->result);

				generate_stack_var_push(block, target, op->left, measuring);

				generate_call(target, rt_support_set_ivar, measuring);
			}
			break;

		case B_GET_CONST:
			{
				generate_stack_push(target, op->right, measuring);
				generate_stack_var_push(block, target, op->left, measuring);

				generate_call(target, rt_support_get_const, measuring);

				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_SET_CONST:
			{
				generate_stack_var_push(block, target, op->right, measuring);
				generate_stack_push(target, op->left, measuring);
				generate_stack_var_push(block, target, op->result, measuring);

				generate_call(target, rt_support_set_const, measuring);
			}
			break;

		case B_METHOD:
			{
				struct block *method_block = (struct block *)op->left;
				rt_compiled_block_t compiled = measuring ? 0 : compile_block(method_block);

				generate_stack_push(target, (rt_value)compiled, measuring);
				generate_stack_push(target, op->result, measuring);

				generate_call(target, rt_support_define_method, measuring);
			}
			break;

		case B_CALL:
			{
				if(op->right)
					generate_stack_var_push(block, target, op->right, measuring);
				else
					generate_stack_push(target, 0, measuring);

				generate_stack_var_push(block, target, op->result, measuring);

				GEN_BYTE(0xBA); // mov edx,
				GEN_DWORD(op->left);

				generate_call(target, rt_support_call, measuring);

				if(kv_A(block->vector, i - 1)->right)
					generate_stack_pop(target, kv_A(block->vector, i - 1)->right * 4, measuring);
			}
			break;

		case B_SUPER:
			{
				if(op->result)
					generate_stack_var_push(block, target, op->result, measuring);
				else
					generate_stack_push(target, 0, measuring);

				GEN_BYTE(0x57); // push edi (self)

				generate_stack_var_push(block, target, (rt_value)block->super_module_var, measuring);
                generate_stack_var_push(block, target, (rt_value)block->super_name_var, measuring);

				generate_call(target, rt_support_super, measuring);

				if(kv_A(block->vector, i - 1)->right)
					generate_stack_pop(target, kv_A(block->vector, i - 1)->right * 4, measuring);
			}
			break;

		case B_ARGS:
			{
				if(op->left)
				{
					GEN_BYTE(0x54); // push esp
					generate_stack_push(target, op->right, measuring);
				}
			}
			break;

		case B_STRING:
			{
				generate_stack_push(target, op->left, measuring);
				generate_call(target, rt_support_define_string, measuring);

				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_INTERPOLATE:
			{
				generate_call(target, rt_support_interpolate, measuring);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4, measuring);

				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_ARRAY:
			{
				generate_call(target, rt_support_array, measuring);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4, measuring);

				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_PUSH:
			generate_stack_var_push(block, target, op->result, measuring);
			break;

		case B_PUSH_RAW:
		case B_PUSH_IMM:
			generate_stack_push(target, op->result, measuring);
			break;

		case B_SELF:
			generate_var_store_self(block, target, op->result, measuring);
			break;

		case B_LOAD:
			generate_var_load(block, target, op->result, measuring);
			break;

		case B_STORE:
			generate_var_store(block, target, op->result, measuring);
			break;

		case B_JMP:
			{
				GEN_BYTE(0xE9);
				GEN_DWORD(label_target(start, op->result) - ((size_t)*target - 1) - 5);
			}
			break;

		case B_RAISE_RETURN:
			{
				generate_stack_push(target, op->left, measuring);
				generate_stack_var_push(block, target, op->result, measuring);
				generate_call(target, rt_support_return, measuring);
			}
			break;

		case B_RAISE_BREAK:
			{
				generate_stack_push(target, op->right, measuring);
				generate_stack_push(target, op->left, measuring);
				generate_stack_var_push(block, target, op->result, measuring);
				generate_call(target, rt_support_break, measuring);
			}
			break;

		case B_RETURN:
			{
				generate_var_load(block, target, op->result, measuring);

				// Jump to epilogue
				GEN_BYTE(0xE9);
				GEN_DWORD(label_target(start, (rt_value)block->epilog) - ((size_t)*target - 1) - 5);
			}
			break;

		case B_JMPNE:
		case B_JMPT:
			{
				GEN_BYTE(0x0F);
				GEN_BYTE(0x85);
				GEN_DWORD(label_target(start, op->result) - ((size_t)*target - 2) - 6);
			}
			break;

		case B_JMPE:
		case B_JMPF:
			{
				GEN_BYTE(0x0F);
				GEN_BYTE(0x84);
				GEN_DWORD(label_target(start, op->result) - ((size_t)*target - 2) - 6);
			}
			break;

		case B_CMP:
			{
				generate_var_load(block, target, op->result, measuring);
				generate_var_cmp(block, target, op->left, measuring);
			}
			break;

		case B_TEST:
			{
				generate_var_load(block, target, op->result, measuring);

				// and eax, ~RT_FALSE

				GEN_BYTE(0x25);
				GEN_DWORD(~RT_FALSE);
			}
			break;

		case B_MOV:
			{
				generate_var_load(block, target, op->left, measuring);
				generate_var_store(block, target, op->result, measuring);
			}
			break;

		case B_MOV_IMM:
			generate_var_store_imm(block, target, op->result, op->left, measuring);
			break;

		case B_ENSURE_RET:
			{
				GEN_BYTE(0x81); // cmp dword [ebp - 12], 0
				GEN_BYTE(0x7D);
				GEN_BYTE(-12);
				GEN_DWORD(0);

				GEN_BYTE(0x74); // jz +1
				GEN_BYTE(1);

				GEN_BYTE(0xC3); // ret
			}
			break;

		case B_HANDLER:
			{
				if(op->result == block->current_exception_block_id - 1)
				{
					GEN_BYTE(0xFF);
					GEN_BYTE(0x4D);
					GEN_BYTE(-8);
				}
				else if(op->result == block->current_exception_block_id + 1)
				{
					GEN_BYTE(0xFF);
					GEN_BYTE(0x45);
					GEN_BYTE(-8);
				}
				else
				{
					GEN_BYTE(0xC7);
					GEN_BYTE(0x45);
					GEN_BYTE(-8);

					GEN_DWORD(op->result);
				}

				block->current_exception_block_id = op->result;
			}
			break;

		case B_REDO:
			{
				if(block->require_exceptions)
				{
					// Restore exception handler index
					GEN_BYTE(0xC7);
					GEN_BYTE(0x45);
					GEN_BYTE(-8);
					GEN_DWORD(-1);
				}

				// Jump to prolog
				GEN_BYTE(0xE9);
				GEN_DWORD((size_t)block->prolog - ((size_t)*target - 1) - 5);
			}
			break;

		case B_LABEL:
			{
				if(measuring)
					op->result = (rt_value)*target;

				switch(op->left)
				{
					case L_FLUSH:
						{
							if(block->type == S_CLOSURE)
							{
								GEN_BYTE(0x8B); // mov esi, dword [ebp + get_stack_index(block->closure_var)]
								GEN_BYTE(0x75);
								GEN_BYTE((char)get_stack_index(block, (rt_value)block->closure_var));
							}

							if(block->self_ref > 0)
							{
								GEN_BYTE(0x8B); // mov edi, dword [ebp + 8]
								GEN_BYTE(0x7D);
								GEN_BYTE(8);
							}

							if(block->heap_vars)
							{
								GEN_BYTE(0x8B); // mov ebx, dword [ebp + get_stack_index(block->scope_var)]
								GEN_BYTE(0x5D);
								GEN_BYTE((char)get_stack_index(block, (rt_value)block->scope_var));
							}

						}
						break;

					default:
						break;
				}
			}

		default:
			break;
	}
}

static inline void generate_block(struct block *block, uint8_t *start, uint8_t **target, bool measuring)
{
	size_t stack_vars = block->var_count[V_LOCAL] + block->var_count[V_TEMP];

	GEN_BYTE(0x55); // push ebp
	GEN_BYTE(0x89); // mov esp, ebp
	GEN_BYTE(0xE5);

	struct block_data *data = block->data;

	if(block->require_exceptions)
	{
		block->current_exception_block_id = -1;

		/*
		 * Generate seh_frame struct
		 *

		typedef struct struct seh_frame {
			struct struct seh_frame *prev;
			void *handler;
			size_t handling;
			size_t handler_index;
			struct block_data *block;
			size_t old_ebp;
		} struct rt_seh_frame;

		 */

		// old_ebp is already pushed

		// block @ ebp - 4
		generate_stack_push(target, (size_t)data, measuring);

		// block_index @ ebp - 8
		generate_stack_push(target, -1, measuring);

		// handling @ ebp - 12
		generate_stack_push(target, 0, measuring);

		// handler @ ebp - 16
		generate_stack_push(target, (size_t)&rt_support_seh_handler, measuring);

		// prev @ ebp - 20
		GEN_BYTE(0x64); // push dword [fs:0]
		GEN_BYTE(0xFF);
		GEN_BYTE(0x35);
		GEN_DWORD(0);

		/*
		 * Append the struct to the linked list
		 */
		GEN_BYTE(0x64); // mov dword [fs:0], esp
		GEN_BYTE(0x89);
		GEN_BYTE(0x25);
		GEN_DWORD(0);
	}

	if(stack_vars > 0)
	{
		GEN_BYTE(0x81); // add esp,
		GEN_BYTE(0xEC);
		GEN_DWORD(stack_vars * 4);
	}

	if(block->require_exceptions)
	{
		GEN_BYTE(0x56); // push esi
		GEN_BYTE(0x57); // push edi
		GEN_BYTE(0x53); // push ebx
	}
	else
	{
		if(block->type == S_CLOSURE)
			GEN_BYTE(0x56); // push esi

		if(block->self_ref > 0)
			GEN_BYTE(0x57); // push edi

		if(block->heap_vars)
			GEN_BYTE(0x53); // push ebx
	}

	if(block->type == S_CLOSURE)
	{
		GEN_BYTE(0x89); // mov esi, eax
		GEN_BYTE(0xC6);

		if(block->require_exceptions)
			generate_var_store(block, target, (rt_value)block->closure_var, measuring);
	}

	if(block->self_ref > 0)
	{
		GEN_BYTE(0x8B); // mov edi, dword [ebp + 8]
		GEN_BYTE(0x7D);
		GEN_BYTE(8);
	}

	if(block->heap_vars)
	{
		generate_stack_push(target, block->var_count[V_HEAP] * 4, measuring);
		generate_call(target, rt_support_alloc_scope, measuring);

		GEN_BYTE(0x89); // mov ebx, eax
		GEN_BYTE(0xC3);

		if(block->require_exceptions)
			generate_var_store(block, target, (rt_value)block->scope_var, measuring);
	}

	if(block->super_module_var)
	{
		GEN_BYTE(0x89); // mov eax, ecx
		GEN_BYTE(0xC8);

		generate_var_store(block, target, (rt_value)block->super_module_var, measuring);

		GEN_BYTE(0x89); // mov eax, edx
		GEN_BYTE(0xD0);

		generate_var_store(block, target, (rt_value)block->super_name_var, measuring);
	}

	if(block->block_parameter)
	{
		GEN_BYTE(0x8B); // mov eax, dword [ebp + 12]
		GEN_BYTE(0x45);
		GEN_BYTE((char)12);

		generate_var_store(block, target, (rt_value)block->block_parameter, measuring);
	}

	if(kv_size(block->parameters))
	{
		GEN_BYTE(0x8B); // mov ecx, dword [ebp + 20]
		GEN_BYTE(0x4D);
		GEN_BYTE((char)20);

		for(size_t i = 0; i < kv_size(block->parameters); i++)
		{
			struct variable *var = kv_A(block->parameters, i);

			// Load to eax
			GEN_BYTE(0x8B); // mov eax, dword [ecx + 12]
			GEN_BYTE(0x41);
			GEN_BYTE((char)((kv_size(block->parameters) - i - 1) * 4));

			generate_var_store(block, target, (rt_value)var, measuring);
		}
	}

	block->prolog = *target;

	for(size_t i = 0; i < kv_size(block->vector); i++)
		generate_instruction(block, kv_A(block->vector, i), i, start, target, measuring);

	if(block->require_exceptions)
	{
		data->epilog = (void *)label_target(start, (rt_value)block->epilog);

		GEN_BYTE(0x8B); // mov ecx, dword [ebp - 20] ; Load previous exception frame
		GEN_BYTE(0x4D);
		GEN_BYTE(-20);

		GEN_BYTE(0x64); // mov dword [fs:0], ecx
		GEN_BYTE(0x89);
		GEN_BYTE(0x0D);
		GEN_DWORD(0);

		GEN_BYTE(0x5B); // pop ebx
		GEN_BYTE(0x5F); // pop edi
		GEN_BYTE(0x5E); // pop esi
	}
	else
	{
		if(block->heap_vars)
			GEN_BYTE(0x5F); // pop ebx

		if(block->self_ref > 0)
			GEN_BYTE(0x5F); // pop edi

		if(block->type == S_CLOSURE)
			GEN_BYTE(0x5E); // pop esi
	}

	GEN_BYTE(0x89);
	GEN_BYTE(0xEC);
	GEN_BYTE(0x5D);
	GEN_BYTE(0xC2);

	GEN_WORD(16); // ret 16
}

rt_compiled_block_t compile_block(struct block *block)
{
	if(block->require_exceptions)
	{
		if(block->heap_vars)
			block->scope_var = block_get_var(block);

		if(block->type == S_CLOSURE)
			block->closure_var = block_get_var(block);
	}

	uint8_t *target = 0;
	struct block_data *data = block->data;
	size_t stack_vars = block->var_count[V_LOCAL] + block->var_count[V_TEMP];

	/*
	 * Calculate the size of the block
	 */
	generate_block(block, 0, &target, true);

	/*
	 * Allocate the block
	 */
	rt_compiled_block_t result = rt_code_heap_alloc((size_t)target);

	/*
	 * Setup data structures
	 */
	if(block->require_exceptions)
	{
		block->current_exception_block_id = -1;

		/*
		 * Setup block data structure
		 */

		kv_mov(data->exception_blocks, block->exception_blocks);

		for(size_t i = 0; i < kv_size(data->exception_blocks); i++)
		{
			struct exception_block *exception_block = kv_A(data->exception_blocks, i);

			exception_block->ensure_label = (void *)(exception_block->ensure_label ? label_target(result, (rt_value)exception_block->ensure_label) : 0);

			for(size_t j = 0; j < kv_size(exception_block->handlers); j++)
			{
				struct exception_handler *handler = kv_A(exception_block->handlers, j);

				switch(handler->type)
				{
					case E_RUNTIME_EXCEPTION:
					case E_CLASS_EXCEPTION:
					case E_FILTER_EXCEPTION:
						{
							struct runtime_exception_handler *exception_handler = (struct runtime_exception_handler *)handler;
							exception_handler->rescue_label = (void *)label_target(result, (rt_value)exception_handler->rescue_label);
						}
						break;

					default:
						break;
				}
			}
		}

		/*
		 * Translate break target labels
		 */

		for(size_t i = 0; i < block->break_targets; i++)
			data->break_targets[i] = (void *)label_target(result, (rt_value)data->break_targets[i]);

		data->local_storage = stack_vars * 4;
	}

	/*
	 * Generate the code
	 */

	target = (uint8_t *)result;

	generate_block(block, (uint8_t *)result, &target, false);

	#ifdef WIN32
		#ifdef DEBUG
			printf(";\n; compiled block %x\n;\n", (rt_value)block);

			disassembly_symbol_vector_t symbols;

			kv_init(symbols);

			if(data)
			{
				struct disassembly_symbol block_data = {data, "block_data"};

				kv_push(struct disassembly_symbol *, symbols, &block_data);
			}

			dump_code((void *)result, target - (unsigned char *)result, &symbols);

			kv_destroy(symbols);

			printf("\n\n");
		#endif
	#endif

	return result;
}
