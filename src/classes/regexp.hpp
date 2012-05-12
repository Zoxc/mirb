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
		public:
			Regexp(Class *instance_of) : Object(Value::Regexp, instance_of) {}

			real_pcre *re;
			CharArray pattern;

			static Regexp *allocate(const CharArray &pattern);
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(pattern);
			}
			
			static void initialize();
	};
};
