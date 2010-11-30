#pragma once
#include "../common.hpp"

namespace Mirb
{
	class MemStream
	{
		private:
			void *memory;
			size_t _capacity;

			void check_size()
			{
				mirb_debug_assert(size() <= _capacity && "Too much has been written");
			}
		public:
			MemStream(void *memory, size_t capacity) : memory(memory), _capacity(capacity), position(memory) {}

			void *position;
		
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
				return (uint8_t *)position - (uint8_t *)memory;
			}

			template<typename T> void write(T var)
			{
				*(T *)position = var;
				position = (void *)((uint8_t *)position + sizeof(T));
				check_size();
			}
			
			void u8(uint8_t var)
			{
				write(var);
			}

			void i8(int8_t var)
			{
				write(var);
			}
			
			void u16(uint16_t var)
			{
				write(var);
			}
			
			void i16(int16_t var)
			{
				write(var);
			}
			
			void u32(uint32_t var)
			{
				write(var);
			}

			void i32(int32_t var)
			{
				write(var);
			}
			
			void u64(uint64_t var)
			{
				write(var);
			}

			void i64(int64_t var)
			{
				write(var);
			}

			void u(size_t var)
			{
				write(var);
			}

			void *reserve(size_t bytes)
			{
				void *result = position;

				position = (void *)((uint8_t *)position + bytes);
				check_size();
				
				return result;
			}
	};
};
