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
			Tuple<Module> *scope;
			value_t block;
			size_t argc;
			value_t *argv;
			value_t *vars;
			const char *ip;
			Frame *prev;
			Tuple<> *scopes;

			CharArray inspect();
	};

	value_t evaluate_block(Frame &frame);
};

