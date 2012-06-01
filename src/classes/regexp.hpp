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
			static value_t source(Regexp *obj);
			static value_t escape(String *str);
			static value_t rb_initialize(Regexp *obj, value_t pattern);
			static value_t rb_allocate(Class *instance_of);
			static value_t rb_match(Regexp *obj, String *string);
			static value_t rb_pattern(Regexp *self, String *str);
			static value_t case_equal(Regexp *self, value_t other);
			

		public:
			static const size_t vector_size = 16 * 3;

			Regexp(Class *instance_of) : Object(Type::Regexp, instance_of), re(nullptr) {}

			static const bool finalizer = true;

			~Regexp();

			real_pcre *re;
			CharArray pattern;

			void compile_pattern();
			
			static CharArray gsub(const CharArray &input, Regexp *pattern, const CharArray &replacement, bool &changed);

			static Regexp *allocate(const CharArray &pattern);
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(pattern);
			}
			
			int match(const CharArray &input, int *ovector, size_t offset);
			
			template<typename F1, typename F2> void split(const CharArray &input, F1 data, F2 split)
			{
				compile_pattern();
		
				int ovector[vector_size];

				size_t prev = 0;
		
				while(true)
				{
					int groups = match(input, ovector, prev);

					if(groups <= 0)
					{
						if(prev < input.size())
							data(input.copy(prev, input.size() - prev));

						return;
					}
					else
					{
						int start = ovector[0];
						int stop = ovector[1];

						for (int i = 0; i < groups; i++) {
							start = std::min(start, ovector[2 * i]);
							stop = std::max(stop, ovector[2 * i + 1]);
						}

						if((size_t)stop == prev)
						{
							split(start, stop);

							data(input.copy(prev, 1));

							prev += 1;
						}
						else
						{
							data(input.copy(prev, (size_t)start - prev));

							split(start, stop);

							prev = stop;
						}
					}
				}
			}
	
			
			static void initialize();
	};
};
