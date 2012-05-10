#include "collector.hpp"
#include "classes/array.hpp"
#include "classes/hash.hpp"
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
#include "classes/regexp.hpp"
#include "classes/proc.hpp"
#include "classes/symbol.hpp"
#include "classes/io.hpp"
#include "classes/file.hpp"

namespace Mirb
{
	void dummy();
	
	namespace Accesser
	{
		struct CharArray
		{
			static inline char_t *const &data(const Mirb::CharArray &input)
			{
				return input.data;
			}
			
			static inline bool static_data(const Mirb::CharArray &input)
			{
				return input.static_data;
			}
		};
	};
	
	template<Value::Type type> struct FreeClass
	{
		typedef void Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static void func(value_t value)
		{
			static_cast<Class *>(value)->~Class();
		}
	};
	
	template<typename F> void each_root(F mark)
	{
		context->mark(mark);

		{
			OnStackBlock<false> *on_stack = OnStackBlock<false>::current;
		
			while(on_stack)
			{
				value_t **refs = on_stack->get_refs();

				for(size_t i = 0; i < on_stack->size; ++i)
				{
					if(*refs[i] != nullptr)
						mark(*refs[i]);
				}

				on_stack = on_stack->prev;
			}
		}
		
		{
			OnStackBlock<true> *on_stack = OnStackBlock<true>::current;
		
			while(on_stack)
			{
				CharArray **refs = on_stack->get_refs();

				for(size_t i = 0; i < on_stack->size; ++i)
					mark(*refs[i]);

				on_stack = on_stack->prev;
			}
		}

		if(current_exception)
			mark(current_exception);

		Frame *frame = current_frame;

		while(frame)
		{
			Block *code = frame->code;

			mark(frame->code);
			mark(frame->obj);
			mark(frame->name);
			mark(frame->scope);
			mark(frame->block);

			if(frame->scopes)
				mark(frame->scopes);
			
			if(code->executor == &evaluate_block && frame->vars)
			{
				for(size_t i = 0; i < code->var_words; ++i)
				{
					mark(frame->vars[i]);
				}
			}

			frame = frame->prev;
		}
	}
	
	template<Value::Type type> struct SizeOf
	{
		typedef size_t Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static size_t func(value_t)
		{
			return sizeof(Class);
		}
	};
	
	template<> struct SizeOf<Value::InternalValueTuple>
	{
		typedef size_t Result;

		static size_t func(value_t obj)
		{
			return static_cast<Tuple<> *>(obj)->entries * sizeof(value_t) + sizeof(Tuple<>);
		}
	};

	template<> struct SizeOf<Value::InternalTuple>
	{
		typedef size_t Result;

		static size_t func(value_t obj)
		{
			return static_cast<Tuple<Object> *>(obj)->entries * sizeof(Object *) + sizeof(Tuple<Object>);
		}
	};

	template<> struct SizeOf<Value::InternalVariableBlock>
	{
		typedef size_t Result;

		static size_t func(value_t obj)
		{
			return static_cast<VariableBlock *>(obj)->bytes;
		}
	};

	size_t size_of_value(value_t value);
	
	typedef Allocator<ValueMapPair *, AllocatorBase>::Storage ValueMapPairStorage;
	typedef Allocator<value_t, AllocatorBase>::Storage ValueStorage;
};
