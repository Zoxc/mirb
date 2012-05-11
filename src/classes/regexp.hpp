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
		public:
			Regexp(Class *instance_of) : Object(Value::Regexp, instance_of) {}

			real_pcre *re;

			static Regexp *allocate(const char_t *pattern, size_t length);

			static void initialize();
	};
};
