#include "collector.hpp"
#include "document.hpp"
#include "runtime.hpp"
#include "tree/tree.hpp"
#include "on-stack.hpp"
#include "collector-common.hpp"
#include <set>


namespace Mirb
{
	void thread_value(value_t *node);
	void thread_pointer(value_t *node);
	void thread_data(value_t *node, value_t temp);

	template<class T> static void template_thread(T *&value)
	{
		thread_pointer(reinterpret_cast<value_t *>(&value));
	};

	template<> void template_thread<Value>(Value *&value)
	{
		thread_value(&value);
	}

	struct ThreadFunc
	{
		void operator()(const ValueStorage &storage)
		{
			if(storage)
				template_thread(*const_cast<decltype(storage.array) *>(&storage.array));
		}

		void operator()(const CharArray &string)
		{
			char_t * const&data = Accesser::CharArray::data(string);

			if(data && !Accesser::CharArray::static_data(string))
				thread_data((value_t *)&data, &VariableBlock::from_memory(data));
		}

		template<class T> void operator()(T *&value)
		{
			template_thread(value);
		}
	};

	template<Type::Enum type> struct ThreadClass
	{
		typedef void Result;
		typedef typename Type::ToClass<type>::Class Class;

		static void func(value_t value)
		{
			ThreadFunc func;

			static_cast<Class *>(value)->template mark<ThreadFunc>(func);
		}
	};

	void thread_value(value_t *node)
	{
		if((*node)->object_ref())
			thread_pointer(node);
	}

	void thread_data(value_t *node, value_t temp)
	{
		mirb_debug_assert((*node != (value_t)Value::list_end) && "We have seen this field before");

		temp->assert_alive();
		mirb_debug_assert(temp->marked == true);
		mirb_debug_assert((temp->*Value::mark_list) == Value::list_end);

		*node = (value_t)(temp->*Value::thread_list);
		temp->*Value::thread_list = node;
	}

	void thread_pointer(value_t *node)
	{
		value_t temp = *node;

		thread_data(node, temp);
	}

	template<bool second> void update(value_t node, value_t free)
	{
		if(second)
		{
			mirb_debug_assert(node->marked == true);

			node->marked = false;
		}

		if(node->value_type == Type::InternalVariableBlock)
			free = (value_t)((size_t)free + sizeof(VariableBlock));

		value_t *current = (node->*Value::thread_list);

		while(current != Value::list_end)
		{
			mirb_debug_assert(((value_t)current != *current) && "Infinite loop detected!");

			value_t *temp = (value_t *)*current;
			*current = free;
			current = temp;
		}

		(node->*Value::thread_list) = Value::list_end;
	}

	void move(value_t p, value_t new_p, size_t size)
	{
		mirb_debug(mirb_debug_assert(p->block_size == size));

		if(p != new_p)
			std::memmove(new_p, p, size);
	}

	void thread_children(value_t obj)
	{
		Type::action<ThreadClass>(obj->value_type, obj);
	}

	struct RegionWalker
	{
		Collector::Region *region;
		value_t pos;

		RegionWalker()
		{
			region = Collector::regions.first;
		}

		void test(value_t obj prelude_unused)
		{
			mirb_debug(mirb_debug_assert(obj->magic == Value::magic_value));
			mirb_debug_assert(obj->value_type != Type::None);
			mirb_debug(mirb_debug_assert(obj->alive));
			mirb_debug_assert((obj->*Value::mark_list) == Value::list_end);
		}

		bool load()
		{
			pos = (value_t)region->data();

			if(pos == (value_t)region->pos)
				return step_region();

			test(pos);

			return true;
		}

		bool step_region()
		{
		start:
			region = region->entry.next;

			if(region)
			{
				pos = (value_t)region->data();

				if(pos == (value_t)region->pos)
					goto start;

				test(pos);

				return true;
			}
			else
				return false;
		}

		bool jump(value_t target, Collector::Region *region)
		{
			if(!region)
				return false;

			this->region = region;
			this->pos = target;

			mirb_debug_assert(pos != (value_t)region->pos);
			mirb_debug_assert(region->contains(pos));

			test(pos);

			return true;
		}

		bool step(size_t size)
		{
			mirb_debug_assert(region->contains(pos));

			value_t next = (value_t)((size_t)pos + size);

			mirb_debug_assert(region->contains(next));

			if((size_t)next == (size_t)region->pos)
				return step_region();
			else
			{
				test(next);

				mirb_debug_assert(((size_t)next > (size_t)region) && ((size_t)next <= (size_t)region->pos));

				pos = next;

				return true;
			}
		}
	};

	template<bool backward> struct RegionAllocator
	{
		Collector::Region *region;
		value_t pos;

		RegionAllocator()
		{
			region = Collector::regions.first;
			pos = (value_t)region->data();
		}

		void update()
		{
			if(backward)
				region->pos = (char_t *)pos;
		}

		value_t allocate(size_t size)
		{
		start:
			value_t result = pos;

			mirb_debug_assert(region->area_contains(pos));

			value_t next = (value_t)((size_t)pos + size);

			if((size_t)next > (size_t)region->end)
			{
				update();

				region = region->entry.next;

				if(region)
				{
					pos = (value_t)region->data();

					goto start;
				}
				else
					mirb_debug_abort("Ran out of free space!");
			}

			mirb_debug_assert(size <= (size_t)region->end);
			mirb_debug_assert(region->area_contains(next));

			pos = next;

			return result;
		}
	};

	void Collector::update_forward()
	{
		ThreadFunc func;

		each_root(func);

		RegionAllocator<false> free;
		RegionWalker obj;

		size_t size;
#ifdef MIRB_GC_SKIP_BLOCKS
		value_t prev = nullptr;
#endif
		bool more;

		obj.load();

		do
		{
			value_t i = obj.pos;
			size = size_of_value(i);

			mirb_debug_assert((obj.pos >= free.pos) || obj.region != free.region);

			more = obj.step(size);

			if(i->marked)
			{
				value_t pos = free.allocate(size);

				update<false>(i, pos);

				thread_children(i);

#ifdef MIRB_GC_SKIP_BLOCKS
				prev = nullptr;
#endif
			}
			else
			{
				Type::action<FreeClass>(i->value_type, i);

#ifdef MIRB_GC_SKIP_BLOCKS
				if(prev)
				{
					prev->value_type = Type::FreeBlock;

					#ifdef DEBUG
						prev->block_size = sizeof(FreeBlock);
					#endif

					((FreeBlock *)prev)->next = (void *)obj.region;
					prev->*Value::thread_list = (value_t *)obj.pos;
				}
				else
				{
					prev = i;
				}
#endif
			}

		}
		while(more);

		for(auto i = heap_list.begin(); i != heap_list.end(); ++i)
		{
			(*i)->assert_valid_base();
			mirb_debug_assert(((*i)->*Value::mark_list) == Value::list_end);

			if(i().marked)
			{
				update<false>(*i, *i);
				thread_children(*i);
			}
		}

		for(auto i = symbol_pool_list.begin(); i != symbol_pool_list.end(); ++i)
		{
			update<false>(*i, *i);
			thread_children(*i);
		}
	}

	void Collector::update_backward()
	{
		RegionAllocator<true> free;
		RegionWalker obj;

		size_t size;

		obj.load();

		do
		{
		start:
			value_t i = obj.pos;
			size = size_of_value(i);

			if(i->marked)
			{
				value_t pos = free.allocate(size);

				update<true>(i, pos);
				move(i, pos, size);

			}
			else
			{
				#ifdef DEBUG
					mirb_debug_assert(i->alive == true);
					i->alive = false;
				#endif

#ifdef MIRB_GC_SKIP_BLOCKS
				if(i->value_type == Type::FreeBlock)
				{
					if(prelude_unlikely(!obj.jump((value_t)(i->*Value::thread_list), (Region *)((FreeBlock *)i)->next)))
						break;
					else
						goto start;
				}
#endif
			}
		}
		while(obj.step(size));

		free.update();

		Collector::current = free.region;

		// Free regions and reset default page allocation count

		{
			Region *current = Collector::current->entry.next;

			Collector::current->entry.next = nullptr;
			Collector::regions.last = Collector::current;

			while(current)
			{
				Region *next = current->entry.next;

				free_region(current);

				current = next;
			}
		}

		pages = ((size_t)Collector::current->end - (size_t)Collector::current) / page_size;

		for(auto heap_obj = heap_list.first; heap_obj;)
		{
			auto next = heap_obj->entry.next;

			heap_obj->assert_valid_base();
			mirb_debug_assert((heap_obj->*Value::mark_list) == Value::list_end);

			if(heap_obj->marked)
				update<true>(heap_obj, heap_obj);
			else
			{
				heap_list.remove(heap_obj);
				Type::action<FreeClass>(heap_obj->value_type, heap_obj);
				std::free((void *)heap_obj);
			}

			heap_obj = next;
		}

		for(auto i = symbol_pool_list.begin(); i != symbol_pool_list.end(); ++i)
			update<true>(*i, *i);
	}

	void Collector::compact()
	{
		update_forward();
		update_backward();

		#ifdef DEBUG_MEMORY
		{

			RegionWalker obj;

			if(!obj.load())
				return;

			do
			{
				mirb_debug_assert(obj.pos->marked == false);
				mirb_debug_assert(((obj.pos)->*Value::Header::thread_list) == Value::Header::list_end);
				mirb_debug_assert(((obj.pos)->*Value::Header::mark_list) == Value::Header::list_end);

				Value::assert_valid(obj.pos);
			}
			while(obj.step(size_of_value(obj.pos)));
		}
		#endif
	};

	void Collector::finalize_regions()
	{
		RegionWalker obj;

		if(!obj.load())
			return;

		do
		{
			obj.pos->assert_valid();
			Type::action<FreeClass>(obj.pos->value_type, obj.pos);
		}
		while(obj.step(size_of_value(obj.pos)));
	};
};
