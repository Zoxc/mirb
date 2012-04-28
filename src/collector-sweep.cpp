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
		auto unmark = [&](value_t obj) {
			Value::assert_valid_base(obj);
			mirb_debug_assert((obj->*Value::Header::mark_list) == nullptr);
			
			obj->alive = obj->marked;
			obj->marked = false;
		};

		for(auto obj = heap_list.first; obj;)
		{
			auto next = obj->entry.next;
			
			unmark(obj);

			if(!obj->alive)
			{
				heap_list.remove(obj);
				std::free((void *)obj);
			}

			obj = next;
		}
		
	#ifndef VALGRIND
		for(auto i = regions.begin(); i != regions.end(); ++i)
		{
			value_t obj = i().data();

			while((size_t)obj < (size_t)i().pos)
			{
				unmark(obj);

				obj = (value_t)((size_t)obj + size_of_value(obj));
			}
		}
	#endif

		symbol_pool.each_value([&](Symbol *symbol) {
			symbol->marked = false;
			symbol->alive = true;
		});
	}
};
