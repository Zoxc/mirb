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
	void Collector::sweep()
	{
		for(auto obj = heap_list.first; obj;)
		{
			auto next = obj->entry.next;
			
			Value::assert_valid_base(obj);
			mirb_debug_assert((obj->*Value::Header::mark_list) == Value::Header::list_end);
			
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
