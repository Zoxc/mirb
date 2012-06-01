#include "collector.hpp"
#include "document.hpp"
#include "runtime.hpp"
#include "tree/tree.hpp"
#include "on-stack.hpp"
#include "collector-common.hpp"

namespace Mirb
{
	value_t mark_list = (value_t)Value::list_end;
	value_t mark_parent;
	
	bool mark_value(value_t obj);
	bool mark_pointer(value_t obj);
	
	template<class T> static bool template_mark(T *value)
	{
		return mark_pointer(value);
	};

	template<void(*callback)()> struct MarkFunc
	{
		void operator()(const ValueStorage &storage)
		{
			if(template_mark(storage.array))
				callback();
		}

		void operator()(const CharArray &string)
		{
			char_t *data = Accesser::CharArray::data(string);

			if(data && !Accesser::CharArray::static_data(string))
				if(mark_pointer(&VariableBlock::from_memory((const void *)data)))
					callback();
		}
		
		template<class T> void operator()(T *value)
		{
			if(template_mark(value))
				callback();
		}
	};

	template<Type::Enum type> struct MarkClass
	{
		typedef void Result;
		typedef typename Type::ToClass<type>::Class Class;

		static void func(value_t value)
		{
			MarkFunc<&dummy> func;

			if(!Value::immediate(type))
				static_cast<Class *>(value)->mark(func);
		}
	};
				
	template<> bool template_mark<Value>(Value *value)
	{
		return mark_value(value);
	}

	bool mark_pointer(value_t obj)
	{
		obj->assert_alive();

		mirb_debug_assert((obj->*Value::thread_list) == Value::list_end);

		if(!obj->marked)
		{
			mirb_debug_assert((obj->*Value::mark_list) == Value::list_end);

			obj->marked = true;
			
			#ifdef DEBUG
				obj->refs = mark_parent;
			#endif

			(obj->*Value::mark_list) = (value_t *)mark_list;
			mark_list = obj;

			return true;
		}
		else
			return false;
	}
	
	bool mark_value(value_t obj)
	{
		if(obj->object_ref())
			return mark_pointer(obj);
		else
			return false;
	}
	
	void mark_children()
	{
		#ifdef DEBUG
			mark_parent = nullptr;
		#endif

		while(mark_list != (value_t)Value::list_end)
		{
			value_t current = mark_list;
			mark_list = (value_t)(mark_list->*Value::mark_list);
			(current->*Value::mark_list) = Value::list_end;

			#ifdef DEBUG
				mark_parent = current;
			#endif

			Type::action<MarkClass>(current->type(), current);
			
			#ifdef DEBUG
				current->refs = 0;
			#endif
		}
	}

	void Collector::mark()
	{
		MarkFunc<&mark_children> func;

		each_root(func);

		symbol_pool.mark_content(func);
	}
};
