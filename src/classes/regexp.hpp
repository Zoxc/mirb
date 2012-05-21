#pragma once
#include "../char-array.hpp"
#include "../collector.hpp"
#include "../context.hpp"
#include "class.hpp"

struct real_pcre;

namespace Mirb
{
	class Regexp:
		public Object
	{
		private:
			static value_t to_s(Regexp *obj);
			static value_t rb_initialize(Regexp *obj, value_t pattern);
			static value_t rb_allocate(Class *instance_of);
			static value_t match(Regexp *obj, String *string);
		public:
			Regexp(Class *instance_of) : Object(Value::Regexp, instance_of), re(nullptr) {}

			static const bool finalizer = true;

			~Regexp();

			real_pcre *re;
			CharArray pattern;

			bool compile_pattern();

			static Regexp *allocate(const CharArray &pattern);
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(pattern);
			}
			
			static void initialize();
	};
};
