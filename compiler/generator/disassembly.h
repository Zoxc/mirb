#include "../../globals.h"

typedef struct {
	void *address;
	char *symbol;
} disassembly_symbol_t;

#define DISASSEMBLY_SYMBOL(name) {&name, #name}

typedef kvec_t(disassembly_symbol_t *) disassembly_symbol_vector_t;

#ifdef WINDOWS
    void dump_hex(unsigned char* address, int length);
    void dump_instruction(DISASM* instruction, int length, disassembly_symbol_vector_t *vector);
    void dump_code(unsigned char* address, int length, disassembly_symbol_vector_t *vector);
#endif
