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

	template<Type::Enum type> struct NaiveMarkClass
	{
		typedef void Result;
		typedef typename Type::ToClass<type>::Class Class;

		static void func(value_t value)
		{
			NaiveMarkFunc func;

			if(!Value::immediate(type))
				static_cast<Class *>(value)->mark(func);
		}
	};
	
	template<> bool template_naive_mark<Value>(Value *value)
	{
		return naive_mark_value(value);
	}

	bool naive_mark_pointer(value_t obj)
	{
		obj->assert_alive();
		mirb_runtime_assert((obj->*Value::thread_list) == Value::list_end);
		mirb_runtime_assert((obj->*Value::mark_list) == Value::list_end);

		if(!obj->marked)
		{

			obj->marked = true;

			Type::action<NaiveMarkClass>(obj->type(), obj);

			return true;
		}
		else
			return false;
	}
	
	bool naive_mark_value(value_t obj)
	{
		if(obj->object_ref())
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
