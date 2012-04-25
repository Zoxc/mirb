#include "collector.hpp"
#include "classes/array.hpp"
#include "classes/class.hpp"
#include "classes/string.hpp"
#include "classes/proc.hpp"
#include "classes/symbol.hpp"
#include "classes/exceptions.hpp"
#include "document.hpp"
#include "tree/tree.hpp"

namespace Mirb
{
	size_t Collector::pages = 16;
	Collector::Region *Collector::current;
	FastList<Collector::Region> Collector::regions;

	FastList<PinnedHeader> Collector::pinned_object_list;
	
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
	
	bool marked(value_t p)
	{
		return true;
	}

	void Collector::thread(value_t *node)
	{
		value_t temp = *node;

		if(Value::object_ref(temp))
		{
			*node = (value_t)temp->data;
			temp->data = node;
		}
	}

	void Collector::update(value_t node, value_t free)
	{
		value_t *current = node->data;

		while(current)
		{
			value_t *temp = (value_t *)*current;
			*current = free;
			current = temp;
		}

		node->data = nullptr;
	}
	
	void Collector::move(value_t p, value_t new_p)
	{
	}

	void Collector::update_forward()
	{
		value_t bottom;
		value_t top;

		//for each root: thread(node);

		value_t free = bottom;
		value_t p = bottom;

		while(p < top)
		{
			if(marked(p))
			{
				Collector::update(p, free);
				//for each child pointer: thread(child);

				free += sizeof(Object);
			}

			p += sizeof(Object);
		}
	}
	
	void Collector::update_backward()
	{
		value_t bottom;
		value_t top;

		value_t free = bottom;
		value_t p = bottom;

		while(p < top)
		{
			if(marked(p))
			{
				update(p, free);
				move(p, free);

				free += sizeof(Object);
			}

			p += sizeof(Object);
		}
	}

	void Collector::collect()
	{
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
};
