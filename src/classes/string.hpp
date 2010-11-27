#pragma once
#include "../char-array.hpp"
#include "object.hpp"

namespace Mirb
{
	class String:
		public Object
	{
		public:
			String(value_t instance_of) : Object(Value::String, instance_of) {}
			String(CharArray &char_array) : Object(Value::String, class_ref), string(char_array) {}
			String(const char_t *c_str, size_t length) : Object(Value::String, class_ref), string(c_str, length) {}
			
			std::string get_string()
			{
				return string.get_string();
			}
			
			CharArray string;
			char_t *c_str;
			size_t length;
			
			static value_t from_symbol(Symbol *symbol);
			static value_t from_string(const char *c_str);
			
			static value_t class_ref;
	};
};
