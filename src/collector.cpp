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
	Platform::BenchmarkResult Collector::bench;
	size_t Collector::collections = 0;
	size_t Collector::region_count = 0;
	size_t Collector::region_free_count = 0;
	size_t Collector::region_allocs_since_collection = 0;
	uint64_t Collector::memory = 0;
	uint64_t Collector::total_memory = 0;
	bool Collector::pending = false;
	bool Collector::enable_interrupts = false;
	size_t Collector::pages = 32;
	Collector::Region *Collector::current;
	List<Collector::Region> Collector::regions;

	std::atomic<bool> pending_exception;
	
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

		VariableBlock *result;

		if(bytes > Collector::max_region_alloc)
		{
			result = new (std::malloc(bytes)) VariableBlock(bytes);
			heap_list.append(result);
		}
		else
		{
			result = new (Collector::allocate_simple(bytes)) VariableBlock(bytes);

			#ifdef VALGRIND
				Collector::heap_list.append(result);
			#endif
		}
		
		#ifdef DEBUG		
			result->block_size = bytes;
		#endif

		return result->data();
	}

	void *Collector::reallocate(void *memory, size_t old_size, size_t bytes)
	{
		#ifdef DEBUG
			if(memory)
			{
				auto &block = VariableBlock::from_memory(memory);

				(&block)->assert_valid();

				size_t old_block_size = block.bytes - sizeof(VariableBlock);

				mirb_debug_assert(old_block_size >= old_size);
			}
		#endif

		void *result = allocate(bytes);

		if(memory)
			memcpy(result, memory, old_size);
			
		return result;
	}

	void dummy()
	{
	};
	
	size_t size_of_value(value_t value)
	{
		size_t size = Type::action<SizeOf>(value->value_type, value);

		mirb_debug(mirb_debug_assert(size == value->block_size));

		return size;
	}

	template<class T> struct Aligned
	{
		template<size_t size> struct Test
		{
			static_assert((size & object_ref_mask) == 0, "Wrong size for class T");
			static_assert(size >= sizeof(FreeBlock), "Too small size for class T");
		};

		typedef Test<sizeof(T)> Run;
	};

	template<Type::Enum type> struct TestSize
	{
		typedef void Result;
		typedef Aligned<typename Type::ToClass<type>::Class> Run;

		static void func(bool) {}
	};
	
	void Collector::test_sizes()
	{
		typedef Aligned<Region> Run;

		Type::action<TestSize, bool>(Type::None, true);
	}

	void Collector::collect()
	{
		region_allocs_since_collection = 0;
		total_memory += memory;
		memory = 0;

		if(enable_interrupts)
		{
			bool expected = true;
		
			if(pending_exception.compare_exchange_strong(expected, false) && expected)
				raise(context->interrupt_class, "Aborted");
		}
		
		Platform::benchmark(bench, [&] {
			mark();

			#ifdef VALGRIND
				sweep();
			#else
				compact();
			#endif
		});

		#ifdef DEBUG_MEMORY
		{
			Frame *frame = context->frame;

			while(frame)
			{
				for(size_t i = 0; i < frame->argc; ++i)
					frame->argv[i]->assert_valid();

				frame = frame->prev;
			}
		}
		#endif

		collections++;
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
		region_free_count++;
		
		Platform::free_region((void *)region, (size_t)region->end - (size_t)region);
	}
	
	void *Collector::get_region(size_t bytes)
	{
		Region *region;
		char_t *result;

		size_t max_page_alloc = 1;

		if(prelude_unlikely(bytes > max_page_alloc * page_size))
		{
			Region *region = allocate_region(bytes + sizeof(Region));

			result = region->pos;

			region->pos = region->end;

			return result;
		}
		else
		{
			if(memory > (512 + collections * 128) * page_size)
				pending = true;
		}

		region = allocate_region(pages * page_size);
		pages += pages >> 1;

		result = region->pos;

		region->pos = result + bytes;
		current = region;

		return result;
	}

	void Collector::initialize()
	{
		current = allocate_region(pages * page_size);
	}

	void Collector::finalize()
	{
		for(auto obj = heap_list.first; obj;)
		{
			auto next = obj->entry.next;
			
			Type::action<FreeClass>(obj->type(), obj);

			std::free(obj);

			obj = next;
		}

	#ifndef VALGRIND
		finalize_regions();

		for(auto region = regions.first; region;)
		{
			auto next = region->entry.next;

			free_region(region);

			region = next;
		}
	#endif
	}
};
