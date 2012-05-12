#include "collector.hpp"
#include "document.hpp"
#include "runtime.hpp"
#include "tree/tree.hpp"
#include "on-stack.hpp"
#include "collector-common.hpp"

namespace Mirb
{
	bool naive_mark_value(value_t obj);
	bool naive_mark_pointer(value_t obj);
	
	template<class T> static bool template_naive_mark(T *value)
	{
		return naive_mark_pointer(value);
	};

	struct NaiveMarkFunc
	{
		void operator()(const ValueStorage &storage)
		{
			template_naive_mark(storage.array);
		}

		void operator()(const CharArray &string)
		{
			char_t *data = Accesser::CharArray::data(string);

			if(data && !Accesser::CharArray::static_data(string))
				naive_mark_pointer(&VariableBlock::from_memory((const void *)data));
		}
		
		template<class T> void operator()(T *value)
		{
			template_naive_mark(value);
		}
	};

	template<Value::Type type> struct NaiveMarkClass
	{
		typedef void Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static void func(value_t value)
		{
			NaiveMarkFunc func;

			if(!Value::immediate(type))
				static_cast<Class *>(value)->mark(func);
		}
	};
	
	template<> bool template_naive_mark<Value::Header>(Value::Header *value)
	{
		return naive_mark_value(value);
	}

	bool naive_mark_pointer(value_t obj)
	{
		Value::assert_alive(obj);
		mirb_runtime_assert((obj->*Value::Header::thread_list) == Value::Header::list_end);
		mirb_runtime_assert((obj->*Value::Header::mark_list) == Value::Header::list_end);

		if(!obj->marked)
		{

			obj->marked = true;

			Value::virtual_do<NaiveMarkClass>(Value::type(obj), obj);

			return true;
		}
		else
			return false;
	}
	
	bool naive_mark_value(value_t obj)
	{
		if(Value::object_ref(obj))
			return naive_mark_pointer(obj);
		else
			return false;
	}
	
	void Collector::naive_mark()
	{
		NaiveMarkFunc func;

		each_root(func);

		symbol_pool.mark_content(func);
	}
};
