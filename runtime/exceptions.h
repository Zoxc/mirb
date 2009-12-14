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

struct exception_block {
	size_t parent_index;
	struct exception_block *parent;
	kvec_t(struct exception_handler *) handlers;
	void *block_label;
	void *ensure_label;
};

struct rt_frame {
	struct rt_frame *prev;
	void *handler;
	size_t handling;
	size_t block_index;
	struct block_data *block;
	size_t old_ebp;
};

#ifdef WIN32
	#define RT_SEH_RUBY 0x4D520000
#else
	extern __thread struct rt_frame *rt_current_handler;
#endif

enum rt_exception_type {
	E_RUBY_EXCEPTION,
	E_RETURN_EXCEPTION,
	E_BREAK_EXCEPTION,
	E_THROW_EXCEPTION
};

void __attribute__((noreturn)) rt_exception_raise(rt_value exception);
