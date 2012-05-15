#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Array:
		public Object
	{
		private:
			static value_t allocate(Class *instance_of);
			static value_t push(Array *self, size_t argc, value_t argv[]);
			static value_t pop(Array *self);
			static value_t shift(Array *self);
			static value_t unshift(Array *self, size_t argc, value_t argv[]);
			static value_t to_s(Array *self);
			static value_t plus(Array *self, Array *other);
			static value_t length(Array *self);
			static value_t each(Array *self, value_t block);
			static value_t get(Array *self, value_t index, value_t size);
			static value_t set(Array *self, size_t index, value_t value);
			static value_t join(Array *self, String *sep);

		public:
			Array(Class *instance_of) : Object(Value::Array, instance_of) {}
			Array() : Object(Value::Array, context->array_class) {}

			Vector<value_t, AllocatorBase, Allocator> vector;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				vector.mark(mark);
			}

			template<typename F> static void parse(const char_t *input, size_t length, F func)
			{
				const char_t *end = input + length;

				auto is_white = [&] {
					return (*input >= 1 && *input <= 32);
				};

				auto skip_white = [&] {
					while(is_white() && input < end)
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
