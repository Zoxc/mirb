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
			static value_t rb_get(String *self, value_t index);
			static value_t inspect(String *self);
			static value_t to_s(value_t self);
			static value_t concat(String *self, String *other);
			static value_t equal(String *self, String *other);
			static value_t downcase(String *self);
			static value_t downcase_self(String *self);
			static value_t upcase(String *self);
			static value_t upcase_self(String *self);
			
		public:
			String(Class *instance_of) : Object(Value::String, instance_of) {}
			String(const CharArray &char_array) : Object(Value::String, auto_cast(context->string_class)), string(char_array) {}
			String(const char_t *c_str, size_t length) : Object(Value::String, auto_cast(context->string_class)), string(c_str, length) {}
			
			template<size_t length> String(const char (&string)[length]) : Object(Value::String, auto_cast(context->string_class)), string(string) {}
			
			std::string get_string()
			{
				return string.get_string();
			}
			
			static value_t get(const CharArray &string);

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
