#pragma once
#include "common.hpp"
#include "char-array.hpp" 
#include "allocator.hpp" 

namespace Mirb
{
	class Document:
		public PinnedHeader
	{
		public:
			Document() : PinnedHeader(Value::InternalDocument) {}
			
			static const bool finalizer = true;

			~Document()
			{
				std::free((void *)data);
			};

			const char_t *data;
			size_t length;
			CharArray name;

			void copy(const char_t *data, size_t length) prelude_nonnull(2);

			template<typename F> void mark(F mark)
			{
				mark(name);
			}
	};
};
