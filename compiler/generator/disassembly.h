#include "../../globals.h"

struct disassembly_symbol {
	void *address;
	char *symbol;
};

#define DISASSEMBLY_SYMBOL(name) {&name, #name}

VEC_DEFAULT(struct disassembly_symbol *, disassembly_symbols)

typedef vec_t(disassembly_symbols) disassembly_symbol_vector_t;

#ifdef BEAENGINE
    void dump_hex(unsigned char* address, int length);
    void dump_instruction(DISASM* instruction, int length, disassembly_symbol_vector_t *vector);
    void dump_code(unsigned char* address, int length, disassembly_symbol_vector_t *vector);
#endif
