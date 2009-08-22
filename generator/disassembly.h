#include "../globals.h"

#ifdef WINDOWS
    void dump_hex(unsigned char* address, int length);
    void dump_instruction(DISASM* instruction, int length);
    void dump_code(unsigned char* address, int length);
#endif
