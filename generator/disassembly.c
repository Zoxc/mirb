#include "disassembly.h"
#include "../../runtime/support.h"
#include "../../runtime/x86.h"

disassembly_symbol_t disassembly_symbols[] = {
	DISASSEMBLY_SYMBOL(rt_support_closure),
	DISASSEMBLY_SYMBOL(rt_support_define_class),
	DISASSEMBLY_SYMBOL(rt_support_define_module),
	DISASSEMBLY_SYMBOL(rt_support_define_method),
	DISASSEMBLY_SYMBOL(rt_support_lookup_method),
	DISASSEMBLY_SYMBOL(rt_support_upval_create),
	DISASSEMBLY_SYMBOL(rt_support_get_ivar),
	DISASSEMBLY_SYMBOL(rt_support_set_ivar),
	DISASSEMBLY_SYMBOL(rt_support_get_upval),
	DISASSEMBLY_SYMBOL(rt_support_set_upval),
	DISASSEMBLY_SYMBOL(rt_seh_handler),
	DISASSEMBLY_SYMBOL(rt_support_seal_upval),
	DISASSEMBLY_SYMBOL(rt_support_interpolate),
	DISASSEMBLY_SYMBOL(rt_support_array),
	DISASSEMBLY_SYMBOL(rt_support_get_const),
	DISASSEMBLY_SYMBOL(rt_support_set_const),
	DISASSEMBLY_SYMBOL(rt_support_define_string)
};

char *find_symbol(void *address, disassembly_symbol_vector_t *vector)
{
	for(int i = 0; i < sizeof(disassembly_symbols) / sizeof(disassembly_symbol_t); i++)
	{
		if(disassembly_symbols[i].address == address)
			return disassembly_symbols[i].symbol;
	}

	if(vector)
	{
		for(int i = 0; i < kv_size(*vector); i++)
		{
			if(kv_A(*vector, i)->address == address)
				return kv_A(*vector, i)->symbol;
		}
	}

	return 0;
}

#ifdef WINDOWS
void dump_hex(unsigned char* address, int length)
{
	int MinLength = 7 - length;

	int Index = 0;

	while(Index < length)
		printf(" %.2X", (size_t)*(address + Index++));

	while(MinLength-- > 0)
		printf("   ");
}

void dump_instruction(DISASM* instruction, int length, disassembly_symbol_vector_t *vector)
{
	printf("0x%08X:", instruction->VirtualAddr == 0 ? (size_t)instruction->EIP : (size_t)instruction->VirtualAddr);

	dump_hex((unsigned char*)(size_t)instruction->EIP, length == -1 ? 1 : length);

	if(length == -1)
		printf(" ??");
	else
		printf(" %s", instruction->CompleteInstr);

	char *symbol = find_symbol((void *)(size_t)instruction->Instruction.AddrValue, vector);

	if(!symbol)
		symbol = find_symbol((void *)(size_t)instruction->Instruction.Immediat, vector);

	if(symbol)
		printf(" ;%s", symbol);

	printf("\n");
}

void dump_code(unsigned char* address, int length, disassembly_symbol_vector_t *vector)
{
	DISASM Instruction;

	DWORD Flags;

	size_t InstructionLength;

	RT_ASSERT(VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &Flags)); // Terminate("DumpCode: Unable to get read permissions from address 0x%08X (%u).\n", address, GetLastError());

	memset(&Instruction, 0, sizeof(DISASM));
	Instruction.Options = NasmSyntax | PrefixedNumeral;
	Instruction.EIP = (size_t)address;

	do
	{
		InstructionLength = Disasm(&Instruction);

		dump_instruction(&Instruction, InstructionLength, vector);

		Instruction.EIP += InstructionLength;
	}
	while((size_t)Instruction.EIP < (size_t)address + length);

	RT_ASSERT(VirtualProtect(address, length, Flags, &Flags)); // Terminate("DumpCode: Unable to restore address 0x%08X (%u).\n", address, GetLastError());
}
#endif
