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
			Exception *exception;
			Tuple<> *scopes;

			Frame() : vars(0), exception(0), scopes(0) {}

			size_t var_count(Block *code);

			template<typename F> void mark(F mark)
			{
				auto code_block = code;

				mark(code);
				mark(obj);
				mark(name);
				mark(scope);
				mark(block);
				
				if(scopes)
					mark(scopes);
			
				if(exception)
					mark(exception);
			
				if(vars)
				{
					for(size_t i = 0; i < var_count(code_block); ++i)
					{
						mark(vars[i]);
					}
				}
			}

			CharArray inspect();
	};

	value_t evaluate_block(Frame &frame);
};

