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

static inline void generate_byte(unsigned char **target, unsigned char byte)
{
	**target = byte;
	(*target)++;
}

static inline void generate_word(unsigned char **target, unsigned short word)
{
	**(unsigned short**)target = word;
	*target += sizeof(word);
}

static inline void generate_dword(unsigned char **target, size_t dword)
{
	**(size_t**)target = dword;
	*target += sizeof(dword);
}

static inline size_t instruction_size(struct block *block, struct opcode *op, size_t i, size_t current)
{
	switch(op->type)
	{
		case B_LABEL:
			{
				op->result = (rt_value)current;

				switch(op->left)
				{
					case L_FLUSH:
						return 3;

					default:
						return 0;
				}
			}

		case B_NOP:
			return 0;

		case B_MOV_IMM:
			return 7;

		case B_MOV:
			return 6;

		case B_CALL:
			return (op->right ? 3 : 5) + 3 + 5 + 5 + 2 + (kv_A(block->vector, i - 1)->right ? 6 : 0);

		case B_PUSH:
			return 3;

		case B_PUSH_RAW:
		case B_PUSH_IMM:
			return 5;

		case B_SELF:
		case B_LOAD:
		case B_STORE:
			return 3;

			return 3;

		case B_TEST:
			return 3 + 5;

		case B_JMP:
			return 5;

		case B_JMPT:
		case B_JMPF:
		case B_JMPE:
		case B_JMPNE:
			return 6;

		case B_MODULE:
			return 5 + 5 + 5 + 1 + 5 + 6;

		case B_CLASS:
			return 5 + 5 + 5 + 5 + 1 + 5 + 6;

		case B_METHOD:
			return 5 + 5 + 5;

		case B_GET_CONST:
			return 5 + 3 + 5 + 3;

		case B_SET_CONST:
			return 3 + 5 + 3 + 5;

		case B_STRING:
			return 5 + 5 + 3;

		case B_INTERPOLATE:
			return 5 + 6 + 3;

		case B_ARRAY:
			return 5 + 6 + 3;

		case B_UPVAL:
			return 3 + 5 + 3;

		case B_CLOSURE:
			return 5 + 5 + 6 + 3;

		case B_GET_UPVAL:
			return 5 + 5 + 3;

		case B_SET_UPVAL:
			return 5 + 3 + 5;

		case B_SEAL:
			return 3 + 5;

		case B_ARGS:
			if(op->left)
				return 1 + 5;
			else
				return 0;

		case B_PUSH_UPVAL:
			if(((struct variable *)op->result)->index)
				return 3;
			else
				return 2;

		case B_GET_IVAR:
			return 5 + 5 + 3;

		case B_SET_IVAR:
			return 5 + 3 + 5;

		case B_CMP:
			return 6;

        case B_ENSURE_RET:
            return (3 + 4) + 2 + 1;

		case B_HANDLER:
			{
				size_t current_id = block->current_exception_block_id;

				block->current_exception_block_id = op->result;

				if(op->result ==current_id - 1)
					return 3;
				else if(op->result ==current_id + 1)
					return 3;
				else
					return 3 + 4;
			}

		case B_RETURN:
			return 3 + 5;

		case B_RAISE_RETURN:
			return 5 + 3 + 5;

		case B_RAISE_BREAK:
			return 5 + 5 + 3 + 5;

		case B_REDO:
			return (block->require_exceptions ? 7 : 0) + 5;

		default:
			break;
	}

	return 0;
}

static inline int get_stack_index(struct block *block, rt_value var)
{
	struct variable *_var = (struct variable *)var;

	int index;

	switch(_var->type)
	{
		case V_BLOCK:
			return 12;

		case V_PARAMETER:
			index = block->var_count[V_LOCAL] + block->var_count[V_TEMP] + _var->index;
			break;

		case V_TEMP:
			index = block->local_offset + block->var_count[V_LOCAL] + _var->index;
			break;

		case V_LOCAL:
			index = block->local_offset + _var->index;
			break;

		default:
			RT_ASSERT(0);
	}

	return -((index + 1) * 4);
}

static inline void generate_call(unsigned char **target, void *function)
{
	generate_byte(target, 0xE8);
	generate_dword(target, (size_t)function - ((size_t)*target - 1) - 5);
}

static inline void generate_stack_push(unsigned char **target, size_t dword)
{
	generate_byte(target, 0x68);
	generate_dword(target, dword);
}

static inline void generate_stack_var_push(struct block *block, unsigned char **target, rt_value var)
{
	generate_byte(target, 0xFF);
	generate_byte(target, 0x75);
	generate_byte(target, (char)get_stack_index(block, var));
}

static inline void generate_stack_pop(unsigned char **target, size_t bytes)
{
	generate_byte(target, 0x81);
	generate_byte(target, 0xC4);
	generate_dword(target, bytes);
}

static inline void generate_instruction(struct block *block, struct opcode *op, size_t i, unsigned char *start, unsigned char **target)
{
	switch(op->type)
	{
		case B_SEAL:
			{
				generate_stack_var_push(block, target, op->result);
				generate_call(target, rt_support_seal_upval);
			}
			break;

		case B_GET_UPVAL:
			{
				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, ((struct variable *)op->left)->index);

				generate_call(target, rt_support_get_upval);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_SET_UPVAL:
			{
				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, ((struct variable *)op->result)->index);

				generate_stack_var_push(block, target, op->left);

				generate_call(target, rt_support_set_upval);
			}
			break;

		case B_PUSH_UPVAL:
			{
				size_t index = ((struct variable *)op->result)->index * 4;

				if(index)
				{
					generate_byte(target, 0xFF); // push dword [esi + index]
					generate_byte(target, 0x76);
					generate_byte(target, ((struct variable *)op->result)->index * 4);
				}
				else
				{
					generate_byte(target, 0xFF); // push dword [esi]
					generate_byte(target, 0x36);
				}
			}
			break;

		case B_CLOSURE:
			{
				generate_stack_push(target, (size_t)compile_block((struct block *)op->left));

				generate_call(target, rt_support_closure);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 3) * 4);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_UPVAL:
			{
				generate_byte(target, 0x8D);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->left));

				generate_call(target, rt_support_upval_create);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_CLASS:
			{
				struct block *class_block = (struct block *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

				generate_stack_push(target, rt_Object);
				generate_stack_push(target, op->result);
				generate_call(target, rt_support_define_class);

				generate_byte(target, 0x50); // push dummy argv
				generate_stack_push(target, 0); // push argc
				generate_stack_push(target, 0); // push block
				generate_byte(target, 0x50); // push obj(eax)

				generate_call(target, compiled);
			}
			break;

		case B_MODULE:
			{
				struct block *class_block = (struct block *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

				generate_stack_push(target, op->result);
				generate_call(target, rt_support_define_module);

				generate_byte(target, 0x50); // push dummy argv
				generate_stack_push(target, 0); // push argc
				generate_stack_push(target, 0); // push block
				generate_byte(target, 0x50); // push obj(eax)

				generate_call(target, compiled);
			}
			break;

		case B_GET_IVAR:
			{
				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, op->left);

				generate_call(target, rt_support_get_ivar);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_SET_IVAR:
			{
				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, op->result);

				generate_stack_var_push(block, target, op->left);

				generate_call(target, rt_support_set_ivar);
			}
			break;

		case B_GET_CONST:
			{
				generate_stack_push(target, op->right);
				generate_stack_var_push(block, target, op->left);

				generate_call(target, rt_support_get_const);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_SET_CONST:
			{
				generate_stack_var_push(block, target, op->right);
				generate_stack_push(target, op->left);
				generate_stack_var_push(block, target, op->result);

				generate_call(target, rt_support_set_const);
			}
			break;

		case B_METHOD:
			{
				struct block *method_block = (struct block *)op->left;
				rt_compiled_block_t compiled = compile_block(method_block);

				generate_stack_push(target, (rt_value)compiled);
				generate_stack_push(target, op->result);
				//generate_byte(target, 0x57); // push edi

				generate_call(target, rt_support_define_method);
			}
			break;

		case B_CALL:
			{
				if(op->right)
					generate_stack_var_push(block, target, op->right);
				else
					generate_stack_push(target, 0);

				generate_stack_var_push(block, target, op->result);

				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, op->left);

				generate_call(target, rt_support_lookup_method);

				generate_byte(target, 0xFF); // call eax
				generate_byte(target, 0xD0);

				if(kv_A(block->vector, i - 1)->right)
					generate_stack_pop(target, kv_A(block->vector, i - 1)->right * 4);
			}
			break;

		case B_ARGS:
			{
				if(op->left)
				{
					generate_byte(target, 0x54); // push esp
					generate_stack_push(target, op->right);
				}
			}
			break;

		case B_STRING:
			{
				generate_stack_push(target, op->left);
				generate_call(target, rt_support_define_string);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_INTERPOLATE:
			{
				generate_call(target, rt_support_interpolate);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_ARRAY:
			{
				generate_call(target, rt_support_array);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_PUSH:
			generate_stack_var_push(block, target, op->result);
			break;

		case B_PUSH_RAW:
		case B_PUSH_IMM:
			generate_stack_push(target, op->result);
			break;

		case B_SELF:
			{
				generate_byte(target, 0x89);
				generate_byte(target, 0x7D);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_LOAD:
			{
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_STORE:
			{
				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_JMP:
			{
				generate_byte(target, 0xE9);
				generate_dword(target, label_target(start, op->result) - ((size_t)*target - 1) - 5);
			}
			break;

		case B_RAISE_RETURN:
			{
				generate_stack_push(target, op->left);
				generate_stack_var_push(block, target, op->result);
				generate_call(target, rt_support_return);
			}
			break;

		case B_RAISE_BREAK:
			{
				generate_stack_push(target, op->right);
				generate_stack_push(target, op->left);
				generate_stack_var_push(block, target, op->result);
				generate_call(target, rt_support_break);
			}
			break;

		case B_RETURN:
			{
				// Load to eax
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));

				// Jump to epilogue
				generate_byte(target, 0xE9);
				generate_dword(target, label_target(start, (rt_value)block->epilog) - ((size_t)*target - 1) - 5);
			}
			break;

		case B_JMPNE:
		case B_JMPT:
			{
				generate_byte(target, 0x0F);
				generate_byte(target, 0x85);
				generate_dword(target, label_target(start, op->result) - ((size_t)*target - 2) - 6);
			}
			break;

		case B_JMPE:
		case B_JMPF:
			{
				generate_byte(target, 0x0F);
				generate_byte(target, 0x84);
				generate_dword(target, label_target(start, op->result) - ((size_t)*target - 2) - 6);
			}
			break;

		case B_CMP:
			{
				// Load to eax
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));

				generate_byte(target, 0x3B); // cmp eax, [ebp + var]
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->left));
			}
			break;

		case B_TEST:
			{
				// Load to eax
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));

				// and eax, ~RT_FALSE

				generate_byte(target, 0x25);
				generate_dword(target, ~RT_FALSE);
			}
			break;

		case B_MOV:
			{
				// Load to eax
				{
					generate_byte(target, 0x8B);
					generate_byte(target, 0x45);
					generate_byte(target, (char)get_stack_index(block, op->left));
				}

				// Store from eax
				{
					generate_byte(target, 0x89);
					generate_byte(target, 0x45);
					generate_byte(target, (char)get_stack_index(block, op->result));
				}
			}
			break;

		case B_MOV_IMM:
			{
				generate_byte(target, 0xC7);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));

				generate_dword(target, op->left);
			}
			break;

		case B_ENSURE_RET:
			{
			    generate_byte(target, 0x81); // cmp dword [ebp - 12], 0
                generate_byte(target, 0x7D);
                generate_byte(target, -12);
                generate_dword(target, 0);

                generate_byte(target, 0x74); // jz +1
                generate_byte(target, 1);

                generate_byte(target, 0xC3); // ret
			}
			break;

		case B_HANDLER:
			{
				if(op->result == block->current_exception_block_id - 1)
				{
					generate_byte(target, 0xFF);
					generate_byte(target, 0x4D);
					generate_byte(target, -8);
				}
				else if(op->result == block->current_exception_block_id + 1)
				{
					generate_byte(target, 0xFF);
					generate_byte(target, 0x45);
					generate_byte(target, -8);
				}
				else
				{
					generate_byte(target, 0xC7);
					generate_byte(target, 0x45);
					generate_byte(target, -8);

					generate_dword(target, op->result);
				}

				block->current_exception_block_id = op->result;
			}
			break;

		case B_REDO:
			{
				if(block->require_exceptions)
				{
					// Restore exception handler index
					generate_byte(target, 0xC7);
					generate_byte(target, 0x45);
					generate_byte(target, -8);
					generate_dword(target, -1);
				}

				// Jump to epilogue
				generate_byte(target, 0xE9);
				generate_dword(target, (size_t)block->prolog - ((size_t)*target - 1) - 5);
			}
			break;

		case B_LABEL:
			{
				switch(op->left)
				{
					case L_FLUSH:
						generate_byte(target, 0x8B); // mov edi, dword [ebp + 8]
						generate_byte(target, 0x7D);
						generate_byte(target, 8);
						break;

					default:
						break;
				}
			}

		default:
			break;
	}
}

rt_compiled_block_t compile_block(struct block *block)
{
	bool require_exceptions = block->require_exceptions;
	size_t block_size = 3;
	size_t stack_vars = block->var_count[V_LOCAL] + block->var_count[V_TEMP] + block->var_count[V_PARAMETER];

	if(require_exceptions)
	{
		if(block->type == S_CLOSURE)
			stack_vars += 1;

		block->current_exception_block_id = -1;

		block_size += 5 + 5 + 5 + 5 + (3 + 4) + (3 + 4) + 3;

		block->local_offset = 5;
	}
	else
		block->local_offset = 0;

	if(stack_vars > 0)
		block_size += 6;

	if(block->type == S_CLOSURE)
		block_size += 2 + (require_exceptions ? 0 : 1);

	if(block->self_ref > 0)
		block_size += 3 + (require_exceptions ? 0 : 1);

	if(block->var_count[V_PARAMETER])
	{
		khash_t(block) *variables = block->variables;

		block_size += 3;

		for(khiter_t k = kh_begin(variables); k != kh_end(variables); ++k)
		{
			if(kh_exist(variables, k) && kh_value(variables, k)->type == V_PARAMETER)
				block_size += 6;
		}
	}

	for(size_t i = 0; i < kv_size(block->vector); i++)
		block_size += instruction_size(block, kv_A(block->vector, i), i, block_size);

	if(require_exceptions)
		block_size += 3 + (3 + 4) + 3;
	else
	{
		if(block->self_ref > 0)
			block_size += 1;

		if(block->type == S_CLOSURE)
			block_size += 1;
	}

	block_size += 6;

	rt_compiled_block_t result = rt_code_heap_alloc(block_size);

	unsigned char *target = (unsigned char *)result;

	generate_byte(&target, 0x55); // push ebp
	generate_byte(&target, 0x89); // mov esp, ebp
	generate_byte(&target, 0xE5);

	struct block_data *data = block->data;

	if(require_exceptions)
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

		/*
		 * Generate seh_frame_t struct
		 *

        typedef struct seh_frame_t {
            struct seh_frame_t *prev;
            void *handler;
            size_t handling;
            size_t handler_index;
            struct block_data *block;
            size_t old_ebp;
        } rt_seh_frame_t;

		 */

		// old_ebp is already pushed

		// block @ ebp - 4
		generate_stack_push(&target, (size_t)data);

		// block_index @ ebp - 8
		generate_stack_push(&target, -1);

		// handling @ ebp - 12
		generate_stack_push(&target, 0);

		// handler @ ebp - 16
		generate_stack_push(&target, (size_t)&rt_support_seh_handler);

		// prev @ ebp - 20
		generate_byte(&target, 0x64); // push dword [fs:0]
		generate_byte(&target, 0xFF);
		generate_byte(&target, 0x35);
		generate_dword(&target, 0);

		/*
		 * Append the struct to the linked list
		 */
		generate_byte(&target, 0x64); // mov dword [fs:0], esp
		generate_byte(&target, 0x89);
		generate_byte(&target, 0x25);
		generate_dword(&target, 0);
	}

	if(stack_vars > 0)
	{
		generate_byte(&target, 0x81); // add esp,
		generate_byte(&target, 0xEC);
		generate_dword(&target, stack_vars * 4);
	}

	if(require_exceptions)
	{
		generate_byte(&target, 0x56); // push esi
		generate_byte(&target, 0x57); // push edi
		generate_byte(&target, 0x53); // push ebx
	}

	if(block->type == S_CLOSURE)
	{
		if(!require_exceptions)
			generate_byte(&target, 0x56); // push esi

		generate_byte(&target, 0x89); // mov esi, eax
		generate_byte(&target, 0xC6);
	}

	if(block->self_ref > 0)
	{
		if(!require_exceptions)
			generate_byte(&target, 0x57); // push edi

		generate_byte(&target, 0x8B); // mov edi, dword [ebp + 8]
		generate_byte(&target, 0x7D);
		generate_byte(&target, 8);
	}

	block->prolog = target;

	if(block->var_count[V_PARAMETER])
	{
		khash_t(block) *variables = block->variables;

		generate_byte(&target, 0x8B);
		generate_byte(&target, 0x4D);
		generate_byte(&target, (char)20);

		for(khiter_t k = kh_begin(variables); k != kh_end(variables); ++k)
		{
			if(kh_exist(variables, k))
			{
				struct variable *var = kh_value(variables, k);

				if(var->type == V_PARAMETER)
				{
					// Load to edx
					generate_byte(&target, 0x8B);
					generate_byte(&target, 0x51);
					generate_byte(&target, (char)((block->var_count[V_PARAMETER] - var->index - 1) * 4));

					// Store from edx
					generate_byte(&target, 0x89);
					generate_byte(&target, 0x55);
					generate_byte(&target, (char)get_stack_index(block, (rt_value)var));
				}
			}
		}
	}

	for(size_t i = 0; i < kv_size(block->vector); i++)
		generate_instruction(block, kv_A(block->vector, i), i, (unsigned char *)result, &target);

	if(require_exceptions)
	{
		data->epilog = (void *)label_target(result, (rt_value)block->epilog);

		generate_byte(&target, 0x8B); // mov ecx, dword [ebp - 20] ; Load previous exception frame
		generate_byte(&target, 0x4D);
		generate_byte(&target, -20);

		generate_byte(&target, 0x64); // mov dword [fs:0], ecx
		generate_byte(&target, 0x89);
		generate_byte(&target, 0x0D);
		generate_dword(&target, 0);

		generate_byte(&target, 0x5B); // pop ebx
		generate_byte(&target, 0x5F); // pop edi
		generate_byte(&target, 0x5E); // pop esi
	}
	else
	{
		if(block->self_ref > 0)
			generate_byte(&target, 0x5F); // pop edi

		if(block->type == S_CLOSURE)
			generate_byte(&target, 0x5E); // pop esi
	}

	generate_byte(&target, 0x89);
	generate_byte(&target, 0xEC);
	generate_byte(&target, 0x5D);
	generate_byte(&target, 0xC2);
	generate_word(&target, 16);

	#ifdef WINDOWS
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

	RT_ASSERT((unsigned char *)result + block_size == target);

	return result;
}
