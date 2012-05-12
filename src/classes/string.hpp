#pragma once
#include "../char-array.hpp"
#include "../collector.hpp"
#include "../context.hpp"
#include "class.hpp"

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
			String(Class *instance_of) : Object(Value::String, instance_of) {}
			String(const CharArray &char_array) : Object(Value::String, auto_cast(context->string_class)), string(char_array) {}
			String(const char_t *c_str, size_t length) : Object(Value::String, auto_cast(context->string_class)), string(c_str, length) {}
			
			template<size_t length> String(const char (&string)[length]) : Object(Value::String, auto_cast(context->string_class)), string(string) {}
			
			std::string get_string()
			{
				return string.get_string();
			}
			
			template<size_t length> static value_t from_literal(const char (&string)[length])
			{
				return auto_cast(Collector::allocate<String>(string));
			}
			
			CharArray string;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(string);
			}
			
			size_t hash()
			{
				if(!hashed)
				{
					hash_value = string.hash();
					hashed = true;
				}

				return hash_value;
			}

			static value_t from_symbol(Symbol *symbol);
			static value_t from_string(const char *c_str);
			
			static void initialize();
	};
};
