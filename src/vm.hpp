#pragma once
#include "common.hpp"
#include "value.hpp"
#include "char-array.hpp"

namespace Mirb
{
	class Block;

	class Frame
	{
		public:
			Block *code;
			value_t obj;
			Symbol *name;
			value_t module;
			value_t block;
			size_t argc;
			value_t *argv;
			value_t *vars;
			const char *ip;
			Frame *prev;

			CharArray inspect();
	};

	class ProcFrame:
		public Frame
	{
		public:
			Tuple &scopes;

			ProcFrame(Tuple &scopes) : scopes(scopes)  {}
	};

	extern Frame *current_frame;

	value_t evaluate_block(Frame &frame);
};

