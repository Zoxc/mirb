#include "allocator.hpp"
#include "collector.hpp"

namespace Mirb
{
	void *Allocator::allocate(size_t bytes)
	{
		bytes += sizeof(VariableBlock);
		bytes = align(bytes, mirb_object_align);
						
		VariableBlock *result = new (Collector::allocate_simple(bytes)) VariableBlock(bytes);

		#ifdef DEBUG		
			result->size = bytes;
		#endif

		#ifdef VALGRIND
			Collector::heap_list.append(result);
		#endif

		return result->data();
	}

	void *Allocator::reallocate(void *memory, size_t old_size, size_t bytes)
	{
		#ifdef DEBUG
			if(memory)
			{
				auto &block = VariableBlock::from_memory(memory);

				Value::assert_valid(&block);

				size_t old_block_size = block.bytes - sizeof(VariableBlock);

				mirb_debug_assert(old_block_size >= old_size);
			}
		#endif

		bytes += sizeof(VariableBlock);
		bytes = align(bytes, mirb_object_align);
						
		VariableBlock *result = new (Collector::allocate_simple(bytes)) VariableBlock(bytes);

		#ifdef DEBUG		
			result->size = bytes;
		#endif

		#ifdef VALGRIND
			Collector::heap_list.append(result);
		#endif

		memcpy(result->data(), memory, old_size);
			
		return result->data();
	}
};
