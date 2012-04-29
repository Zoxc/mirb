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
	
	template<> void template_thread<Value::Header>(Value::Header *&value)
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

		void operator()(const ValueMapPairStorage &storage)
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

	template<Value::Type type> struct ThreadClass
	{
		typedef void Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static void func(value_t value)
		{
			ThreadFunc func;

			static_cast<Class *>(value)->template mark<ThreadFunc>(func);
		}
	};
	
	void thread_value(value_t *node)
	{
		if(Value::object_ref(*node))
			thread_pointer(node);
	}

	void thread_data(value_t *node, value_t temp)
	{
		mirb_debug_assert((*node != (value_t)Value::Header::list_end) && "We have seen this field before");

		Value::assert_alive(temp);
		mirb_debug_assert(temp->marked == true);
		mirb_debug_assert((temp->*Value::Header::mark_list) == Value::Header::list_end);

		*node = (value_t)(temp->*Value::Header::thread_list);
		temp->*Value::Header::thread_list = node;
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

		if(node->type == Value::InternalVariableBlock)
			free = (value_t)((size_t)free + sizeof(VariableBlock));

		value_t *current = (node->*Value::Header::thread_list);

		while(current != Value::Header::list_end)
		{
			mirb_debug_assert(((value_t)current != *current) && "Infinite loop detected!");

			value_t *temp = (value_t *)*current;
			*current = free;
			current = temp;
		}
		
		(node->*Value::Header::thread_list) = Value::Header::list_end;
	}
	
	void move(value_t p, value_t new_p, size_t size)
	{
		mirb_debug(mirb_debug_assert(p->size == size));

		if(p != new_p)
			std::memmove(new_p, p, size);
	}
	
	void thread_children(value_t obj)
	{
		Value::virtual_do<ThreadClass>(Value::type(obj), obj);
	}

	struct RegionWalker
	{
		Collector::Region *region;
		value_t pos;

		RegionWalker()
		{
			region = Collector::regions.first;
		}

		void test(value_t obj)
		{
			Value::assert_alive(obj);
			mirb_debug_assert((obj->*Value::Header::mark_list) == Value::Header::list_end);
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
			region = region->entry.next;

			if(region)
			{
				pos = (value_t)region->data();

				if(pos == (value_t)region->pos)
					return step_region();

				test(pos);

				return true;
			}
			else
				return false;
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
		
		value_t allocate_region(size_t size)
		{
			update();

			region = region->entry.next;

			if(region)
			{
				pos = (value_t)region->data();

				return allocate(size);
			}
			else
				mirb_debug_abort("Ran out of free space!");
		}

		value_t allocate(size_t size)
		{
			value_t result = pos;
			
			mirb_debug_assert(region->contains(pos));
			
			value_t next = (value_t)((size_t)pos + size);

			if((size_t)next > (size_t)region->pos) // TODO: Allocate to region->end
				return allocate_region(size);
			
			mirb_debug_assert(region->contains(next));
			
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
		
		obj.load();

		do
		{
			value_t i = obj.pos;
			size = size_of_value(i);

			mirb_debug_assert((obj.pos >= free.pos) || obj.region != free.region);

			if(i->marked)
			{
				value_t pos = free.allocate(size);

				update<false>(i, pos);

				thread_children(i);
			}
		}
		while(obj.step(size));

		for(auto i = heap_list.begin(); i != heap_list.end(); ++i)
		{
			Value::assert_valid_base(*i);
			mirb_debug_assert(((*i)->*Value::Header::mark_list) == Value::Header::list_end);

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
			value_t i = obj.pos;
			size = size_of_value(i);
			
			if(i->marked)
			{
				value_t pos = free.allocate(size);
				
				update<true>(i, pos);
				move(i, pos, size);
			}
#ifdef DEBUG
			else
			{
				mirb_debug_assert(i->alive == true);
				i->alive = false;
			}
#endif
		}
		while(obj.step(size));

		free.update();

		{
			Region *current = free.region->entry.next;

			while(current)
			{
				current->pos = (char_t *)current->data();
				current = current->entry.next;
			}
		}
		
		for(auto obj = heap_list.first; obj;)
		{
			auto next = obj->entry.next;
			
			Value::assert_valid_base(obj);
			mirb_debug_assert((obj->*Value::Header::mark_list) == Value::Header::list_end);

			if(obj->marked)
				update<true>(obj, obj);
			else
			{
				heap_list.remove(obj);
				std::free((void *)obj);
			}

			obj = next;
		}
		
		for(auto i = symbol_pool_list.begin(); i != symbol_pool_list.end(); ++i)
			update<true>(*i, *i);
	}
	
	void Collector::compact()
	{
		update_forward();
		update_backward();

		#ifdef DEBUG
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
};
