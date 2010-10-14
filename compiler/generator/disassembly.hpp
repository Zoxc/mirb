#include "../../globals.hpp"

struct disassembly_symbol {
	void *address;
	const char *symbol;
};

#define DISASSEMBLY_SYMBOL(name) {(void *)&name, #name}

VEC_DEFAULT(struct disassembly_symbol *, disassembly_symbols)

typedef vec_t(disassembly_symbols) disassembly_symbol_vector_t;

void dump_hex(unsigned char* address, int length);
void dump_code(unsigned char* address, int length, disassembly_symbol_vector_t *vector);
