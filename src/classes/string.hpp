#pragma once
#include "../char-array.hpp"
#include "object.hpp"

namespace Mirb
{
	class String:
		public Object
	{
		private:
			static value_t inspect(value_t obj);
			static value_t to_s(value_t obj);
			static value_t concat(value_t obj, value_t other);
			
		public:
			String(value_t instance_of) : Object(Value::String, instance_of) {}
			String(CharArray &char_array) : Object(Value::String, class_ref), string(char_array) {}
			String(const char_t *c_str, size_t length) : Object(Value::String, class_ref), string(c_str, length) {}
			
			template<size_t length> String(const char (&string)[length]) : Object(Value::String, class_ref), string(string) {}
			
			std::string get_string()
			{
				return string.get_string();
			}
			
			template<size_t length> static value_t from_literal(const char (&string)[length])
			{
				return auto_cast(new (gc) String(string));
			}
			
			CharArray string;
			char_t *c_str;
			size_t length;
			
			static value_t from_symbol(Symbol *symbol);
			static value_t from_string(const char *c_str);
			
			static value_t class_ref;

			static void initialize();
	};
};
