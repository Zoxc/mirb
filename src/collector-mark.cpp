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
	value_t mark_list;
	value_t mark_parent;
	
	bool mark_value(value_t obj);
	bool mark_pointer(value_t obj);
	
	template<class T> static bool template_mark(T *value)
	{
		return mark_pointer(value);
	};

	template<void(*callback)()> struct MarkFunc
	{
		void operator()(const CharArray &string)
		{
			char_t *data = Accesser::CharArray::data(string);

			if(data && !Accesser::CharArray::static_data(string))
				if(mark_pointer(&VariableBlock::from_memory((const void *)data)))
					callback();
		};
		
		template<class T> void operator()(T *value)
		{
			if(template_mark(value))
				callback();
		};
	};

	template<Value::Type type> struct MarkClass
	{
		typedef void Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static void func(value_t value)
		{
			MarkFunc<&dummy> func;

			if(!Value::immediate(type))
				static_cast<Class *>(value)->mark(func);
		}
	};
				
	template<> bool template_mark<Value::Header>(Value::Header *value)
	{
		return mark_value(value);
	}
	
	template<> bool template_mark<void>(void *value)
	{
		return mark_pointer(&VariableBlock::from_memory(value));
	}

	bool mark_pointer(value_t obj)
	{
		assert_valid_skip_mark(obj);

		if(!obj->marked)
		{
			obj->marked = true;
			
			#ifdef DEBUG
				obj->refs = mark_parent;
			#endif

			obj->data = (value_t *)mark_list;
			mark_list = obj;

			return true;
		}
		else
			return false;
	}
	
	bool mark_value(value_t obj)
	{
		if(Value::object_ref(obj))
			return mark_pointer(obj);
		else
			return false;
	}
	
	void mark_children()
	{
		#ifdef DEBUG
			mark_parent = nullptr;
		#endif

		while(mark_list)
		{
			value_t current = mark_list;
			mark_list = (value_t)mark_list->data;
			current->data = nullptr;

			#ifdef DEBUG
				mark_parent = current;
			#endif

			Value::virtual_do<MarkClass>(Value::type(current), current);
		}
	}

	void Collector::mark()
	{
		MarkFunc<&mark_children> func;

		each_root(func);
	}
};
