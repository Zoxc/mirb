#pragma once
#include "common.hpp"

namespace Mirb
{
	class MemStream
	{
		private:
			void *memory;
			void *current;
			size_t _capacity;
		public:
			MemStream(void *memory, size_t capacity) : memory(memory), current(memory), _capacity(capacity) {}
		
			void *pointer()
			{
				return memory;
			}

			size_t capacity()
			{
				return _capacity;
			}

			size_t size()
			{
				return (uint8_t *)current - (uint8_t *)memory;
			}

			template<typename T> void write(T var)
			{
				*(T *)current = var;
				current = (void *)((uint8_t *)current + sizeof(T));
				debug_assert(size() <= _capacity, "Too much has been written");
			}
		
			void b(uint8_t var)
			{
				write(var);
			}

			void w(uint16_t var)
			{
				write(var);
			}
		
			void d(uint32_t var)
			{
				write(var);
			}

			void q(uint64_t var)
			{
				write(var);
			}

			void s(size_t var)
			{
				write(var);
			}
	};
};
