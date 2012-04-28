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

	std::set<value_t *> locs;
	
	void thread_data(value_t *node, value_t temp)
	{
		Value::assert_alive(temp);
		mirb_debug_assert((temp->*Value::Header::mark_list) == nullptr);
		mirb_debug_assert(temp->marked == true);
		mirb_debug_assert((temp->*Value::Header::mark_list) == nullptr);

		*node = (value_t)(temp->*Value::Header::thread_list);
		temp->*Value::Header::thread_list = node;
	}

	void thread_pointer(value_t *node)
	{
		mirb_debug_assert(locs.find(node) == locs.end());
		locs.insert(node);
		
		value_t temp = *node;

		thread_data(node, temp);
	}

	void update(value_t node, value_t free)
	{
		if(node->type == Value::InternalVariableBlock)
			free = (value_t)((size_t)free + sizeof(VariableBlock));

		value_t *current = (node->*Value::Header::thread_list);

		while(current)
		{
			value_t *temp = (value_t *)*current;
			*current = free;
			current = temp;
		}

		(node->*Value::Header::thread_list) = nullptr;
	}
	
	void move(value_t p, value_t new_p)
	{
	}

	struct RegionWalker
	{
		Collector::Region *region;
		value_t pos;

		RegionWalker()
		{
			region = Collector::regions.first;
			pos = (value_t)region->data();
		}

		value_t operator()()
		{
			Value::assert_valid_base(pos);
			mirb_debug_assert((pos->*Value::Header::mark_list) == nullptr);

			return pos;
		}

		bool step_region()
		{
			region = region->entry.next;

			if(region)
			{
				pos = (value_t)region->data();

				return true;
			}
			else
				return false;
		}

		void update()
		{
			region->pos = (char_t *)pos;
		}
		
		void verify()
		{
			mirb_debug_assert(region->pos == (char_t *)pos);
		}

		template<bool free, bool backward> bool step(size_t size)
		{
			size_t next = (size_t)pos + size;

			if(next >= (size_t)region->end)
			{
				if(free)
					update();
				else if(free && backward)
					verify();
				
				if(step_region())
					return step<free, backward>(size);
				else
					return false;
			}
			else
			{
				pos = (value_t)next;

				return true;
			}
		}
	};

	void update_forward()
	{
		locs.clear();

		ThreadFunc func;

		each_root(func);
		
		RegionWalker free;
		RegionWalker pos;

		size_t size;

		do
		{
			value_t i = pos();
			size = size_of_value(i);

			if(i->marked)
			{
				update(i, i);

				Value::virtual_do<ThreadClass>(Value::type(i), i);

				free.step<true, false>(size);
			}
		}
		while(pos.step<false, false>(size));

		free.update();

		symbol_pool.each_value([&](Symbol *symbol) {
			update(symbol, symbol);
		});
	}
	
	void update_backward()
	{
		RegionWalker free;
		RegionWalker pos;

		size_t size;

		do
		{
			value_t i = pos();
			size = size_of_value(i);

			if(i->marked)
			{
				update(i, i);
				move(i, i);

				free.step<true, true>(size);
			}
		}
		while(pos.step<false, true>(size));

		free.verify();

		symbol_pool.each_value([&](Symbol *symbol) {
			update(symbol, symbol);
		});
	}
	
	void Collector::compact()
	{
		update_forward();
		update_backward();
	};
};
