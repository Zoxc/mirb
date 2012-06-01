#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Array:
		public Object
	{
		private:
			static value_t rb_allocate(Class *instance_of);
			static value_t push(Array *self, size_t argc, value_t argv[]);
			static value_t pop(Array *self);
			static value_t concat(Array *self, Array *other);
			static value_t reject_ex(Array *self, value_t block);
			static value_t reject(Array *self, value_t block);
			static value_t compact_ex(Array *self);
			static value_t equal(Array *self, value_t other);
			static value_t replace(Array *self, Array *other);
			static value_t compact(Array *self);
			static value_t shift(Array *self);
			static value_t unshift(Array *self, size_t argc, value_t argv[]);
			static value_t to_s(Array *self);
			static value_t rb_sort(Array *self);
			static bool sort(Array *&array);
			static value_t add(Array *self, Array *other);
			static value_t sub(Array *self, Array *other);
			static value_t length(Array *self);
			static value_t each(Array *self, value_t block);
			static value_t rb_get(Array *self, value_t index, value_t size);
			static value_t rb_set(Array *self, size_t index, value_t value);
			static value_t rb_flatten(Array *self);
			static value_t rb_flatten_ex(Array *self);
			static value_t join(Array *self, String *sep);
			static value_t values_at(Array *self, size_t argc, value_t argv[]);
			
			typedef Vector<value_t, AllocatorBase, Allocator> VectorType;

			void flatten(VectorType &vector, bool &mod);

		public:
			Array(Class *instance_of) : Object(Value::Array, instance_of) {}
			Array() : Object(Value::Array, context->array_class) {}
			
			static Array *allocate();
			static Array *allocate_pair(value_t left, value_t right);

			VectorType vector;
			
			template<typename F> static bool delete_if(Array *self, F func)
			{
				OnStack<1> os(self);

				bool changed = false;
		
				for(size_t i = self->vector.size(); i-- > 0;)
				{
					if(func(self->vector[i]))
					{
						changed = true;
						self->vector.remove(i);
					}
				}

				return changed;
			}

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				vector.mark(mark);
			}
			
			value_t get(size_t index);
			size_t size();

			static value_t rb_delete(Array *array, value_t obj);

			template<typename F> static void parse(const char_t *input, size_t length, F func)
			{
				const char_t *end = input + length;

				auto is_white = [&] {
					return (*input >= 1 && *input <= 32);
				};

				auto skip_white = [&] {
					while(input < end && is_white())
						input++;
				};
				
				auto push = [&] {
					const char_t *start = input;

					while(input < end && !is_white())
						input++;

					std::string out((const char *)start, (size_t)input - (size_t)start);

					func(out);
				};

				while(input < end)
				{
					skip_white();

					if(input < end)
						push();
				}
			}
			
			static void initialize();
	};
};
