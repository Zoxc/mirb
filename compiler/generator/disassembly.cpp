#include <udis86.h>
#include "disassembly.h"
#include "../../runtime/support.h"
#include "../../runtime/x86.h"

struct disassembly_symbol disassembly_symbols[] = {
	DISASSEMBLY_SYMBOL(rt_support_closure),
	DISASSEMBLY_SYMBOL(rt_support_define_class),
	DISASSEMBLY_SYMBOL(rt_support_define_module),
	DISASSEMBLY_SYMBOL(rt_support_define_method),
	DISASSEMBLY_SYMBOL(rt_support_call),
	DISASSEMBLY_SYMBOL(rt_support_super),
	DISASSEMBLY_SYMBOL(rt_support_get_ivar),
	DISASSEMBLY_SYMBOL(rt_support_set_ivar),
	DISASSEMBLY_SYMBOL(rt_support_break),
	DISASSEMBLY_SYMBOL(rt_support_return),
	DISASSEMBLY_SYMBOL(rt_support_alloc_scope),

#ifdef WIN32
	DISASSEMBLY_SYMBOL(rt_support_handler),
#endif

	DISASSEMBLY_SYMBOL(rt_support_interpolate),	DISASSEMBLY_SYMBOL(rt_support_array),
	DISASSEMBLY_SYMBOL(rt_support_get_const),
	DISASSEMBLY_SYMBOL(rt_support_set_const),
	DISASSEMBLY_SYMBOL(rt_support_define_string)
};

char *find_symbol(void *address, disassembly_symbol_vector_t *vector)
{
	for(int i = 0; i < sizeof(disassembly_symbols) / sizeof(struct disassembly_symbol); i++)
	{
		if(disassembly_symbols[i].address == address)
			return disassembly_symbols[i].symbol;
	}

	if(vector)
	{
		for(int i = 0; i < vector->size; i++)
		{
			if(vector->array[i]->address == address)
				return vector->array[i]->symbol;
		}
	}

	return 0;
}

void dump_hex(unsigned char* address, int length)
{
	int MinLength = 7 - length;

	int Index = 0;

	while(Index < length)
		printf(" %.2X", (size_t)*(address + Index++));

	while(MinLength-- > 0)
		printf("   ");
}

void dump_instruction(unsigned char* address, ud_t* ud_obj, disassembly_symbol_vector_t *vector)
{
	printf("0x%08X:", (size_t)ud_insn_off(ud_obj));

	dump_hex(ud_insn_ptr(ud_obj), ud_insn_len(ud_obj));

	printf(" %s", ud_insn_asm(ud_obj));
	
	char *symbol = 0;
	
	for(size_t i = 0; !symbol && i < 3; i++)
	{
		switch(ud_obj->operand[i].type)
		{				
			case UD_OP_IMM:
			case UD_OP_MEM:
				symbol = find_symbol((void *)ud_obj->operand[i].lval.udword, vector);
				break;
			
			case UD_OP_PTR:
				symbol = find_symbol((void *)ud_obj->operand[i].lval.ptr.off, vector);
				break;
			
			case UD_OP_JIMM:
				symbol = find_symbol((void *)((size_t)ud_insn_off(ud_obj) + ud_obj->operand[i].lval.udword + ud_insn_len(ud_obj)), vector);
				break;
			
			default:
				break;
		}
	}
	
	if(symbol)
		printf(" ;%s", symbol);
	
	printf("\n");
}

void dump_code(unsigned char* address, int length, disassembly_symbol_vector_t *vector)
{
	ud_t ud_obj;

	ud_init(&ud_obj);
	ud_set_input_buffer(&ud_obj, address, length);
	ud_set_mode(&ud_obj, 32);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);
	ud_set_pc(&ud_obj, (size_t)address);

#ifdef _WIN32
	DWORD Flags;

	RT_ASSERT(VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &Flags)); // Terminate("DumpCode: Unable to get read permissions from address 0x%08X (%u).\n", address, GetLastError());
#endif
	
	while(ud_disassemble(&ud_obj))
	{
		dump_instruction(address, &ud_obj, vector);
	}
	
#ifdef _WIN32
	RT_ASSERT(VirtualProtect(address, length, Flags, &Flags)); // Terminate("DumpCode: Unable to restore address 0x%08X (%u).\n", address, GetLastError());
#endif
}
