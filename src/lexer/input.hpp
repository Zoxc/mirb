#pragma once
#include "../common.hpp"

namespace Mirb
{
	class Input
	{
		private:
			const char_t *input;

		public:
			enum Characters
			{
				Null = 0,
				CarrigeReturn = 0x13,
				NewLine = 0x10
			};
			
			Input& operator=(const Input& other)
			{
				input = other.input;
				
				return *this;
			}
			
			void set(const char_t *input)
			{
				this->input = input;
			}

			bool in(char_t start, char_t stop)
			{
				return char_in(*input, start, stop);
			}

			static bool char_in(char_t c, char_t start, char_t stop)
			{
				return c >= start && c <= stop;
			}

			const char_t *operator &()
			{
				return input;
			}

			operator char_t()
			{
				return *input;
			}

			char_t operator++()
			{
				return *(++input);
			}

			char_t operator++(int)
			{
				return *(input++);
			}

			char_t operator--()
			{
				return *(--input);
			}

			char_t operator--(int)
			{
				return *(input--);
			}
	};
};
