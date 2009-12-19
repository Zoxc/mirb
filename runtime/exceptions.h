#pragma once
#include "../globals.h"
#include "runtime.h"

/*
 * Exception runtime data
 */

enum exception_handler_type {
	E_RUNTIME_EXCEPTION,
	E_CLASS_EXCEPTION,
	E_FILTER_EXCEPTION,
};

struct exception_handler {
	enum exception_handler_type type;
	struct exception_handler *next;
};

struct runtime_exception_handler {
	struct exception_handler common;
	void *rescue_label;
};

struct class_exception_handler {
	struct runtime_exception_handler common;
};

VEC_INIT(struct exception_handler *, exception_handlers)

struct exception_block {
	size_t parent_index;
	struct exception_block *parent;
	vec_t(exception_handlers) handlers;
	void *block_label;
	void *ensure_label;
};

struct rt_exception_data {
	enum exception_handler_type type;
	void *payload[3];
};

struct rt_frame;

#ifdef WIN_SEH
	typedef EXCEPTION_DISPOSITION( __cdecl *rt_exception_handler_t)(EXCEPTION_RECORD *exception, struct rt_frame *frame, CONTEXT *context, void *dispatcher_context);
#else
	typedef void (*rt_exception_handler_t)(struct rt_frame *frame, struct rt_frame *top, struct rt_exception_data *data, bool unwinding);
#endif

struct rt_frame {
	struct rt_frame *prev;
	rt_exception_handler_t handler;
	size_t handling;
	size_t block_index;
	struct block_data *block;
	size_t old_ebp;
};

#ifdef WIN_SEH
	#define RT_SEH_RUBY 0x4D520000
#else
	extern __thread struct rt_frame *rt_current_frame;
#endif

void __attribute__((noreturn)) rt_exception_raise(struct rt_exception_data *data);

enum rt_exception_type {
	E_RUBY_EXCEPTION,
	E_RETURN_EXCEPTION,
	E_BREAK_EXCEPTION,
	E_THROW_EXCEPTION
};

