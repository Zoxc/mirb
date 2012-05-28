#pragma once
#include "common.hpp"
#include "runtime.hpp"

namespace Mirb
{
	template<void *, bool raise = true, size_t count = 0> class RecursionDetector
	{
		private:
			static prelude_thread RecursionDetector *current;

			value_t val;
			OnStack<1> os;
			RecursionDetector *prev;
			bool error;
		
		public:
			RecursionDetector(value_t val) : val(val), os(this->val), prev(current), error(false)
			{
				auto i = prev;
				size_t c = 0;

				while(i)
				{
					if(i->val == val)
					{
						if(c == count)
						{
							if(raise)
								Mirb::raise(context->argument_error, "Recursive input");
							else
							{
								error = true;
								break;
							}
						}
						else
							c++;
					}

					i = i->prev;
				}

				current = this;
			}

			bool recursion()
			{
				return error;
			}

			~RecursionDetector()
			{
				current = prev;
			}
	};
	
	template<void *func, bool raise, size_t count> prelude_thread RecursionDetector<func, raise, count> *RecursionDetector<func, raise, count>::current = nullptr;
};
