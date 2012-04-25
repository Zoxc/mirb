#pragma once
#include "common.hpp"
#include "char-array.hpp" 
#include "value.hpp" 

namespace Mirb
{
	class MemoryPool;

	class Document:
		public Value::Header
	{
		public:
			Document() : Value::Header(Value::InternalDocument) {}

			const char_t *data;
			size_t length;
			CharArray name;

			void copy(const char_t *data, size_t length);

			template<typename F> void mark(F mark)
			{
				mark(name);
			}
	};
};
