#pragma once
#include "value.hpp"

namespace Mirb
{
	class CharArray;
	
	template<bool string> struct OnStackBase {};
	
	template<> struct OnStackBase<false>
	{
		public:
			value_t **get_refs();

			template<typename T> void push(T *&arg, size_t &index)
			{
				get_refs()[index++] = reinterpret_cast<value_t *>(&arg);
			}

			void push(value_t &arg, size_t &index);
	};

	template<> struct OnStackBase<true>
	{
		public:
			CharArray **get_refs();
			
			void push(CharArray &arg, size_t &index);
			void push(const CharArray &arg, size_t &index);
	};

	template<bool string> class OnStackBlock:
		public OnStackBase<string>
	{
		public:
			OnStackBlock *prev;
			size_t size;

			static OnStackBlock *current;

			OnStackBlock()
			{
				prev = current;
				current = this;
			}

			~OnStackBlock()
			{
				current = prev;
			}
			
			friend class Collector;
	};
	
	template<size_t num_args> class OnStack:
		public OnStackBlock<false>
	{
		public:
			value_t *refs[num_args];
			
			void setup(size_t index prelude_unused)
			{
				size = num_args;

				mirb_debug_assert(index == num_args);
			}

			template<typename Arg1> OnStack(Arg1 &&arg1)
			{
				size_t index = 0;

				push(std::forward<Arg1>(arg1), index);
				setup(index);
			}

			template<typename Arg1, typename Arg2> OnStack(Arg1 &&arg1, Arg2 &&arg2)
			{
				size_t index = 0;
				
				push(std::forward<Arg1>(arg1), index);
				push(std::forward<Arg2>(arg2), index);
				setup(index);
			}
			
			template<typename Arg1, typename Arg2, typename Arg3> OnStack(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3)
			{
				size_t index = 0;
				
				push(std::forward<Arg1>(arg1), index);
				push(std::forward<Arg2>(arg2), index);
				push(std::forward<Arg3>(arg3), index);
				setup(index);
			}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4> OnStack(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3, Arg4 &&arg4)
			{
				size_t index = 0;
				
				push(std::forward<Arg1>(arg1), index);
				push(std::forward<Arg2>(arg2), index);
				push(std::forward<Arg3>(arg3), index);
				push(std::forward<Arg4>(arg4), index);
				setup(index);
			}
	};

	template<size_t num_args> class OnStackString:
		public OnStackBlock<true>
	{
		public:
			CharArray *refs[num_args];
			
			void setup(size_t index prelude_unused)
			{
				size = num_args;

				mirb_debug_assert(index == num_args);
			}

			template<typename Arg1> OnStackString(Arg1 &&arg1)
			{
				size_t index = 0;

				push(std::forward<Arg1>(arg1), index);
				setup(index);
			}

			template<typename Arg1, typename Arg2> OnStackString(Arg1 &&arg1, Arg2 &&arg2)
			{
				size_t index = 0;
				
				push(std::forward<Arg1>(arg1), index);
				push(std::forward<Arg2>(arg2), index);
				setup(index);
			}
			
			template<typename Arg1, typename Arg2, typename Arg3> OnStackString(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3)
			{
				size_t index = 0;
				
				push(std::forward<Arg1>(arg1), index);
				push(std::forward<Arg2>(arg2), index);
				push(std::forward<Arg3>(arg3), index);
				setup(index);
			}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4> OnStackString(Arg1 &&arg1, Arg2 &&arg2, Arg3 &&arg3, Arg4 &&arg4)
			{
				size_t index = 0;
				
				push(std::forward<Arg1>(arg1), index);
				push(std::forward<Arg2>(arg2), index);
				push(std::forward<Arg3>(arg3), index);
				push(std::forward<Arg4>(arg4), index);
				setup(index);
			}
	};
};
