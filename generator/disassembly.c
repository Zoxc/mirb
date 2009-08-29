#include "disassembly.h"

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

void dump_instruction(DISASM* instruction, int length)
{
	printf("0x%08X:", instruction->VirtualAddr == 0 ? (size_t)instruction->EIP : (size_t)instruction->VirtualAddr);

	dump_hex((unsigned char*)(size_t)instruction->EIP, length == -1 ? 1 : length);

	if(length == -1)
		printf(" ??\n");
	else
		printf(" %s\n", instruction->CompleteInstr);
}

void dump_code(unsigned char* address, int length)
{
	DISASM Instruction;

	DWORD Flags;

	size_t InstructionLength;

	assert(VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &Flags)); // Terminate("DumpCode: Unable to get read permissions from address 0x%08X (%u).\n", address, GetLastError());

	memset(&Instruction, 0, sizeof(DISASM));
	Instruction.Options = NasmSyntax | PrefixedNumeral;
	Instruction.EIP = (size_t)address;

	do
	{
		InstructionLength = Disasm(&Instruction);

		dump_instruction(&Instruction, InstructionLength);

		Instruction.EIP += InstructionLength;
	}
	while((size_t)Instruction.EIP < (size_t)address + length);

	assert(VirtualProtect(address, length, Flags, &Flags)); // Terminate("DumpCode: Unable to restore address 0x%08X (%u).\n", address, GetLastError());
}
#endif
