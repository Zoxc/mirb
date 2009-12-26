#pragma once
#include "../runtime/runtime.h"

enum opcode_type {
	B_NOP,
	B_ADD,
	B_SUB,
	B_MUL,
	B_DIV,
	B_MOV,
	B_MOV_IMM,
	B_SELF,
	B_PUSH,
	B_PUSH_IMM,
	B_PUSH_RAW,
	B_CALL,
	B_CALL_ARGS,
	B_CALL_ARGS_POP,
	B_ARGS,
	B_ARGS_POP,
	B_SUPER,
	B_LOAD,
	B_STORE,
	B_TEST,
	B_TEST_PROC,
	B_CMP,
	B_JMPT,
	B_JMPF,
	B_JMPE,
	B_JMPNE,
	B_JMP,
	B_RETURN,
	B_REDO,

	/*
	 * B_LABEL
	 * - result: The place in the generated native code the label have.
	 * - left: The flags of this label.
	 *   - L_FLUSH: This will make the label flush all variables stored in registers.
	 * - right: A per block unique id. (Debug mode only)
	 */
	B_LABEL,

	B_ENSURE_RET,
	B_HANDLER,
	B_RAISE_RETURN,
	B_RAISE_BREAK,
	B_ARRAY,
	B_STRING,
	B_INTERPOLATE,
	B_PUSH_SCOPE,
	B_SEAL,
	B_CLOSURE,
	B_GET_IVAR,
	B_SET_IVAR,
	B_GET_CONST,
	B_SET_CONST,
	B_CLASS,
	B_MODULE,
	B_METHOD
};

enum label_type {
	L_DEFAULT,
	L_FLUSH
};

struct opcode {
	enum opcode_type type;
	rt_value result;
	rt_value left;
	rt_value right;
};

#ifdef DEBUG
	void opcode_print(struct opcode *op);
#endif
