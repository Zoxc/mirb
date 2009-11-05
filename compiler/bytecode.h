#pragma once
#include "../runtime/runtime.h"

typedef enum {
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
	B_ARGS,
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
	B_LABEL,
	B_ENSURE_RET,
	B_HANDLER,
	B_RAISE_RETURN,
	B_RAISE_BREAK,
	B_ARRAY,
	B_STRING,
	B_INTERPOLATE,
	B_UPVAL,
	B_PUSH_UPVAL,
	B_SEAL,
	B_CLOSURE,
	B_GET_UPVAL,
	B_SET_UPVAL,
	B_GET_IVAR,
	B_SET_IVAR,
	B_GET_CONST,
	B_SET_CONST,
	B_CLASS,
	B_MODULE,
	B_METHOD
} opcode_type_t;

typedef enum {
	L_DEFAULT,
	L_FLUSH
} label_type_t;

typedef struct {
	opcode_type_t type;
	rt_value result;
	rt_value left;
	rt_value right;
} opcode_t;

void opcode_print(opcode_t *op);
