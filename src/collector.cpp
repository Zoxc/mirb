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

namespace Mirb
{
	bool Collector::pending = false;
	value_t Collector::mark_list;
	size_t Collector::pages = 16;
	Collector::Region *Collector::current;
	FastList<Collector::Region> Collector::regions;

	FastList<PinnedHeader> Collector::pinned_object_list;

	#ifdef VALGRIND
		LinkedList<Value::Header> Collector::heap_list;
	#endif

	template<class T> struct Aligned
	{
		template<size_t size> struct Test
		{
			static_assert((size & object_ref_mask) == 0, "Wrong size for class T");
		};

		typedef Test<sizeof(T)> Run;
	};

	template<Value::Type type> struct TestSize
	{
		typedef void Result;
		typedef Aligned<typename Value::TypeClass<type>::Class> Run;

		static void func(bool dummy) {}
	};
	
	void Collector::test_sizes()
	{
		typedef Aligned<Region> Run;

		Value::virtual_do<TestSize, bool>(Value::None, true);
	}

	template<Value::Type type> struct SizeOf
	{
		typedef size_t Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static size_t func(value_t obj)
		{
			return sizeof(Class);
		}
	};
	
	template<> struct SizeOf<Value::InternalTuple>
	{
		typedef size_t Result;

		static size_t func(value_t obj)
		{
			return static_cast<Tuple *>(obj)->entries * sizeof(value_t) + sizeof(Tuple);
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

	size_t Collector::size_of_value(value_t value)
	{
		return Value::virtual_do<SizeOf>(value->type, value);
	}
	
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
				{
					Collector::mark_string(*refs[i], [&](value_t data) {
						mark(data);
					});
				}

				on_stack = on_stack->prev;
			}
		}

		if(current_exception)
			mark(current_exception);

		Frame *frame = current_frame;

		while(frame)
		{
			mark(frame->code);
			mark(frame->obj);
			mark(frame->name);
			mark(frame->module);
			mark(frame->block);

			if(frame->scopes)
				mark(frame->scopes);
			
			for(size_t i = 0; i < frame->argc; ++i)
			{
				mark(frame->argv[i]);
			}

			if(frame->code->executor == &evaluate_block && frame->vars)
			{
				for(size_t i = 0; i < frame->code->var_words; ++i)
				{
					mark(frame->vars[i]);
				}
			}

			frame = frame->prev;
		}
	}
	
	template<> void Collector::MarkFunc::mark<Value::Header>(Value::Header *value)
	{
		Collector::mark_value(value);
	};
	
	template<> void Collector::MarkFunc::mark<void>(void *value)
	{
		Collector::mark_pointer(&VariableBlock::from_memory(value));
	};
	
	bool Collector::mark_pointer(value_t obj)
	{
		Value::assert_valid(obj);

		if(!obj->marked)
		{
			obj->marked = true;

			obj->data = (value_t *)mark_list;
			mark_list = obj;

			return true;
		}
		else
			return false;
	}
	
	bool Collector::mark_value(value_t obj)
	{
		if(Value::object_ref(obj))
			return mark_pointer(obj);
		else
			return false;
	}
	
	void Collector::mark()
	{
		each_root([&](value_t root) {
			Collector * __this; // TODO: Remove Visual C++ bug workaround
			(void)__this;

			if(Collector::mark_value(root))
			{
				while(mark_list)
				{
					value_t current = mark_list;
					mark_list = (value_t)mark_list->data;

					Value::virtual_do<MarkClass>(Value::type(current), current);
				}
			}
		});
	}
	
	void Collector::flag()
	{
	#ifdef VALGRIND
		for(value_t obj = heap_list.first; obj;)
		{
			value_t next = obj->entry.next;

			mirb_debug_assert(obj->valid());

			obj->alive = obj->marked;
			obj->marked = false;

			if(!obj->alive)
				heap_list.remove(obj);

			obj = next;
		}
	#else
		for(auto i = regions.begin(); i != regions.end(); ++i)
		{
			value_t obj = i().data();

			while((size_t)obj < (size_t)i().pos)
			{
				Value::assert_valid_internal(obj); // TODO: Replace with Value::assert when compaction is implemented

				obj->alive = obj->marked;
				obj->marked = false;

				obj = (value_t)((size_t)obj + size_of_value(obj));
			}
		}
	#endif
	}
	
	struct ThreadFunc
	{
		void operator()(const value_t &value)
		{
			Collector::thread(const_cast<value_t *>(&value));
		};
		
		void operator()(value_t &value)
		{
			Collector::thread(&value);
		};

		void operator()(const CharArray &string)
		{
			if(string.data && !string.static_data)
				Collector::thread(reinterpret_cast<value_t *>(&const_cast<CharArray *>(&string)->data));
		};
		
		template<class T> void operator()(T *&value)
		{
			Collector::thread(reinterpret_cast<value_t *>(&value));
		};

		template<class T> void operator()(const T *&value)
		{
			Collector::thread(reinterpret_cast<value_t *>(const_cast<void **>(&value)));
		};
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

	void Collector::thread(value_t *node)
	{
		value_t temp = *node;

		if(Value::object_ref(temp))
		{
			*node = (value_t)temp->data;
			temp->data = node;
		}
	}

	void Collector::update(value_t node, value_t free)
	{
		if(node->type == Value::InternalVariableBlock)
			free = (value_t)((size_t)free + sizeof(VariableBlock));

		value_t *current = node->data;

		while(current)
		{
			value_t *temp = (value_t *)*current;
			*current = free;
			current = temp;
		}

		node->data = nullptr;
	}
	
	void Collector::move(value_t p, value_t new_p)
	{
	}

	void Collector::update_forward()
	{
		value_t bottom;
		value_t top;
		/*
		each_root([=](value_t &obj) {
			thread(&obj);
		});
		*/
		value_t free = bottom;
		value_t p = bottom;

		while(p < top)
		{
			if(p->marked)
			{
				Collector::update(p, free);

				Value::virtual_do<ThreadClass>(Value::type(p), p);

				free += sizeof(Object);
			}

			p += sizeof(Object);
		}
	}
	
	void Collector::update_backward()
	{
		value_t bottom;
		value_t top;

		value_t free = bottom;
		value_t p = bottom;

		while(p < top)
		{
			if(p->marked)
			{
				update(p, free);
				move(p, free);

				free += sizeof(Object);
			}

			p += sizeof(Object);
		}
	}

	void Collector::collect()
	{
		mark();
		flag();
	}

	Collector::Region *Collector::allocate_region(size_t bytes)
	{
		char_t *result;
		
		#ifdef WIN32
			result = (char_t *)VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		#else	
			result = (char_t *)mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		#endif
		
		mirb_runtime_assert(result != 0);

		Region *region = new ((void *)result) Region;
		
		region->pos = result + sizeof(Region);
		region->end = result + bytes;

		regions.append(region);

		return region;
	}
	
	void Collector::free_region(Region *region)
	{
		#ifdef WIN32
			VirtualFree((void *)region, 0, MEM_RELEASE);
		#else
			munmap((void *)region, (size_t)region->end - (size_t)region);
		#endif
	}
	
	void *Collector::get_region(size_t bytes)
	{
		Region *region;
		char_t *result;

		size_t max_page_alloc = pages >> 3;

		if(prelude_unlikely(bytes > max_page_alloc * page_size))
		{
			Region *region = allocate_region(bytes + sizeof(Region));

			result = region->pos;

			region->pos = region->end;

			return result;
		}
		else
			pending = true;

		region = allocate_region(pages * page_size);
		pages += max_page_alloc;

		result = region->pos;

		region->pos = result + bytes;
		current = region;

		return result;
	}

	void Collector::initialize()
	{
		current = allocate_region(pages * page_size);
	}
};
