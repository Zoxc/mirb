#include "collector.hpp"
#include "classes/array.hpp"
#include "classes/class.hpp"
#include "classes/string.hpp"
#include "classes/proc.hpp"
#include "classes/symbol.hpp"
#include "classes/exceptions.hpp"
#include "classes/nil-class.hpp"
#include "classes/false-class.hpp"
#include "classes/true-class.hpp"
#include "classes/fixnum.hpp"
#include "classes/method.hpp"
#include "classes/proc.hpp"
#include "classes/symbol.hpp"
#include "document.hpp"
#include "runtime.hpp"
#include "tree/tree.hpp"
#include "on-stack.hpp"
#include "collector-common.hpp"
#include "platform/platform.hpp"
#include <atomic>

namespace Mirb
{
	size_t Collector::collections = 0;
	size_t Collector::region_count = 0;
	unsigned long long Collector::memory = 0;
	bool Collector::pending = false;
	size_t Collector::pages = 32;
	Collector::Region *Collector::current;
	FastList<Collector::Region> Collector::regions;

	std::atomic_bool pending_exception;
	
	#ifdef VALGRIND
		LinkedList<Value::Header> Collector::heap_list;
	#else
		LinkedList<PinnedHeader, PinnedHeader, &PinnedHeader::entry> Collector::heap_list;
	#endif
		
	void Collector::signal()
	{
		pending_exception.store(true);

		prelude_memory_barrier();

		pending = true;
	}

	void *Collector::allocate(size_t bytes)
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

	void *Collector::reallocate(void *memory, size_t old_size, size_t bytes)
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

		if(memory)
			memcpy(result->data(), memory, old_size);
			
		return result->data();
	}

	void dummy()
	{
	};
	
	size_t size_of_value(value_t value)
	{
		size_t size = Value::virtual_do<SizeOf>(value->type, value);

		mirb_debug(mirb_debug_assert(size == value->size));

		return size;
	}

	template<class T> struct Aligned
	{
		template<size_t size> struct Test
		{
			static_assert((size & object_ref_mask) == 0, "Wrong size for class T");
		};

		typedef Test<sizeof(T)> Run;
	};

	template<Value::Type type> struct TestSize
	{
		typedef void Result;
		typedef Aligned<typename Value::TypeClass<type>::Class> Run;

		static void func(bool) {}
	};
	
	void Collector::test_sizes()
	{
		typedef Aligned<Region> Run;

		Value::virtual_do<TestSize, bool>(Value::None, true);
	}

	bool Collector::collect()
	{
		bool expected = true;

		if(pending_exception.compare_exchange_strong(expected, false) && expected)
		{
			raise(context->interrupt_class, "Aborted");

			return true;
		}

		mark();

		#ifdef VALGRIND
			sweep();
		#else
			compact();
		#endif

		#ifdef DEBUG
		{
			Frame *frame = current_frame;

			while(frame)
			{
				for(size_t i = 0; i < frame->argc; ++i)
				{
					Value::assert_valid(frame->argv[i]);
				}

				frame = frame->prev;
			}
		}
		#endif

		collections++;

		return false;
	}

	Collector::Region *Collector::allocate_region(size_t bytes)
	{
		char_t *result = (char_t *)Platform::allocate_region(bytes);

		Region *region = new ((void *)result) Region;
		
		region->pos = result + sizeof(Region);
		region->end = result + bytes;

		region_count++;

		regions.append(region);

		return region;
	}
	
	void Collector::free_region(Region *region)
	{
		Platform::free_region((void *)region, (size_t)region->end - (size_t)region);
	}
	
	void *Collector::get_region(size_t bytes)
	{
		Region *region;
		char_t *result;

		size_t max_page_alloc = 1;// pages >> 3;

		if(prelude_unlikely(bytes > max_page_alloc * page_size))
		{
			Region *region = allocate_region(bytes + sizeof(Region));

			result = region->pos;

			region->pos = region->end;

			return result;
		}
		else
			pending = true;

		region = allocate_region(pages * page_size);
		pages += max_page_alloc;

		result = region->pos;

		region->pos = result + bytes;
		current = region;

		return result;
	}

	void Collector::initialize()
	{
		current = allocate_region(pages * page_size);
	}

	void Collector::free()
	{
		for(auto obj = heap_list.first; obj;)
		{
			auto next = obj->entry.next;

			std::free((void *)obj);

			obj = next;
		}

	#ifndef VALGRIND
		for(auto region = regions.first; region;)
		{
			auto next = region->entry.next;

			free_region(region);

			region = next;
		}
	#endif
	}
};
