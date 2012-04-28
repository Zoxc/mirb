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

namespace Mirb
{
	bool Collector::pending = false;
	size_t Collector::pages = 32;
	Collector::Region *Collector::current;
	FastList<Collector::Region> Collector::regions;
	
	#ifdef VALGRIND
		LinkedList<Value::Header> Collector::heap_list;
	#else
		LinkedList<PinnedHeader, PinnedHeader, &PinnedHeader::entry> Collector::heap_list;
	#endif

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

		static void func(bool dummy) {}
	};
	
	void Collector::test_sizes()
	{
		typedef Aligned<Region> Run;

		Value::virtual_do<TestSize, bool>(Value::None, true);
	}

	void Collector::collect()
	{
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
	}

	Collector::Region *Collector::allocate_region(size_t bytes)
	{
		char_t *result;
		
		#ifdef WIN32
			result = (char_t *)VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		#else	
			result = (char_t *)mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		#endif
		
		mirb_runtime_assert(result != 0);

		Region *region = new ((void *)result) Region;
		
		region->pos = result + sizeof(Region);
		region->end = result + bytes;

		regions.append(region);

		return region;
	}
	
	void Collector::free_region(Region *region)
	{
		#ifdef WIN32
			VirtualFree((void *)region, 0, MEM_RELEASE);
		#else
			munmap((void *)region, (size_t)region->end - (size_t)region);
		#endif
	}
	
	void *Collector::get_region(size_t bytes)
	{
		Region *region;
		char_t *result;

		size_t max_page_alloc = pages >> 3;

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
