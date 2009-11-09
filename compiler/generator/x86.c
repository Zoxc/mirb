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

static inline void generate_stack_var_push(struct block *block, uint8_t **target, rt_value var, bool measuring)
{
	GEN_BYTE(0xFF);
	GEN_BYTE(0x75);
	GEN_BYTE(get_stack_index(block, var));
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
				generate_stack_var_push(block, target, op->result, measuring);
				generate_call(target, rt_support_seal_upval, measuring);
			}
			break;

		case B_GET_HVAR:
			{
				GEN_BYTE(0xB8); // mov eax,
				GEN_DWORD(((struct variable *)op->left)->index);

				generate_call(target, rt_support_get_upval, measuring);

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_SET_HVAR:
			{
				GEN_BYTE(0xB8); // mov eax,
				GEN_DWORD(((struct variable *)op->result)->index);

				generate_stack_var_push(block, target, op->left, measuring);

				generate_call(target, rt_support_set_upval, measuring);
			}
			break;

		case B_PUSH_SCOPE:
			{
				size_t index = ((struct variable *)op->result)->index * 4;

				if(index)
				{
					GEN_BYTE(0xFF); // push dword [esi + index]
					GEN_BYTE(0x76);
					GEN_BYTE(((struct variable *)op->result)->index * 4);
				}
				else
				{
					GEN_BYTE(0xFF); // push dword [esi]
					GEN_BYTE(0x36);
				}
			}
			break;

		case B_CLOSURE:
			{
				generate_stack_push(target, (size_t)compile_block((struct block *)op->left), measuring);

				generate_call(target, rt_support_closure, measuring);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 3) * 4, measuring);

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_UPVAL:
			{
				GEN_BYTE(0x8D);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->left));

				generate_call(target, rt_support_upval_create, measuring);

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_CLASS:
			{
				struct block *class_block = (struct block *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

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
				struct block *class_block = (struct block *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

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

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
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

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
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
				rt_compiled_block_t compiled = compile_block(method_block);

				generate_stack_push(target, (rt_value)compiled, measuring);
				generate_stack_push(target, op->result, measuring);
				//GEN_BYTE(0x57); // push edi

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

				GEN_BYTE(0xB8); // mov eax,
				GEN_DWORD(op->left);

				generate_call(target, rt_support_lookup_method, measuring);

				GEN_BYTE(0xFF); // call eax
				GEN_BYTE(0xD0);

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

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_INTERPOLATE:
			{
				generate_call(target, rt_support_interpolate, measuring);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4, measuring);

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_ARRAY:
			{
				generate_call(target, rt_support_array, measuring);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4, measuring);

				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
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
			{
				GEN_BYTE(0x89);
				GEN_BYTE(0x7D);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_LOAD:
			{
				GEN_BYTE(0x8B);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
			break;

		case B_STORE:
			{
				GEN_BYTE(0x89);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));
			}
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
				// Load to eax
				GEN_BYTE(0x8B);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));

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
				// Load to eax
				GEN_BYTE(0x8B);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));

				GEN_BYTE(0x3B); // cmp eax, [ebp + var]
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->left));
			}
			break;

		case B_TEST:
			{
				// Load to eax
				GEN_BYTE(0x8B);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));

				// and eax, ~RT_FALSE

				GEN_BYTE(0x25);
				GEN_DWORD(~RT_FALSE);
			}
			break;

		case B_MOV:
			{
				// Load to eax
				{
					GEN_BYTE(0x8B);
					GEN_BYTE(0x45);
					GEN_BYTE((char)get_stack_index(block, op->left));
				}

				// Store from eax
				{
					GEN_BYTE(0x89);
					GEN_BYTE(0x45);
					GEN_BYTE((char)get_stack_index(block, op->result));
				}
			}
			break;

		case B_MOV_IMM:
			{
				GEN_BYTE(0xC7);
				GEN_BYTE(0x45);
				GEN_BYTE((char)get_stack_index(block, op->result));

				GEN_DWORD(op->left);
			}
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

				// Jump to epilogue
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
						GEN_BYTE(0x8B); // mov edi, dword [ebp + 8]
						GEN_BYTE(0x7D);
						GEN_BYTE(8);
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

	if(block->type == S_CLOSURE)
	{
		if(!block->require_exceptions)
			GEN_BYTE(0x56); // push esi

		GEN_BYTE(0x89); // mov esi, eax
		GEN_BYTE(0xC6);
	}

	if(block->self_ref > 0)
	{
		if(!block->require_exceptions)
			GEN_BYTE(0x57); // push edi

		GEN_BYTE(0x8B); // mov edi, dword [ebp + 8]
		GEN_BYTE(0x7D);
		GEN_BYTE(8);
	}

	block->prolog = target;

	if(block->block_parameter)
	{
		// Load to edx
		GEN_BYTE(0x8B);
		GEN_BYTE(0x51);
		GEN_BYTE((char)12);

		// Store from edx
		GEN_BYTE(0x89);
		GEN_BYTE(0x55);
		GEN_BYTE((char)get_stack_index(block, (rt_value)block->block_parameter));
	}

	if(kv_size(block->parameters))
	{
		GEN_BYTE(0x8B);
		GEN_BYTE(0x4D);
		GEN_BYTE((char)20);

		for(size_t i = 0; i < kv_size(block->parameters); i++)
		{
			struct variable *var = kv_A(block->parameters, i);

			// Load to edx
			GEN_BYTE(0x8B);
			GEN_BYTE(0x51);
			GEN_BYTE((char)((kv_size(block->parameters) - var->index - 1) * 4));

			// Store from edx
			GEN_BYTE(0x89);
			GEN_BYTE(0x55);
			GEN_BYTE((char)get_stack_index(block, (rt_value)var));
		}
	}

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
		if(block->self_ref > 0)
			GEN_BYTE(0x5F); // pop edi

		if(block->type == S_CLOSURE)
			GEN_BYTE(0x5E); // pop esi
	}

	GEN_BYTE(0x89);
	GEN_BYTE(0xEC);
	GEN_BYTE(0x5D);
	GEN_BYTE(0xC2);

	GEN_WORD(16);
}

rt_compiled_block_t compile_block(struct block *block)
{
	uint8_t *target = 0;
	struct block_data *data = block->data;
	size_t stack_vars = block->var_count[V_LOCAL] + block->var_count[V_TEMP];

	/*
	 * Calculate the size of the block
	 */
	generate_block(block, 0, &target, true);

	printf("block size is %d\n", (size_t)target);

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

	return result;
}
