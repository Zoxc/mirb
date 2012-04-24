#pragma once
#include "common.hpp"
#include "char-array.hpp" 
#include "collector.hpp" 

namespace Mirb
{
	class MemoryPool;

	class Document:
		public ObjectHeader
	{
		public:
			Document() : ObjectHeader(Value::InternalDocument) {}

			const char_t *data;
			size_t length;
			CharArray name;

			void copy(const char_t *data, size_t length);
	};
};
