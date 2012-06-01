#include "collector.hpp"
#include "document.hpp"
#include "runtime.hpp"
#include "tree/tree.hpp"
#include "on-stack.hpp"
#include "collector-common.hpp"

namespace Mirb
{
	void Collector::sweep()
	{
		for(auto obj = heap_list.first; obj;)
		{
			auto next = obj->entry.next;
			
			obj->assert_valid_base();
			mirb_debug_assert((obj->*Value::mark_list) == Value::list_end);
			
			mirb_debug(obj->alive = obj->marked);

			if(!obj->marked)
			{
				heap_list.remove(obj);
				std::free((void *)obj);
			}
			else
				obj->marked = false;

			obj = next;
		}
		
		for(auto i = symbol_pool_list.begin(); i != symbol_pool_list.end(); ++i)
		{
			i().marked = false;
			mirb_debug(i().alive = true);
		}
	}
};
