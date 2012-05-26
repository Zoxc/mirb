#pragma once
#include "value.hpp"
#include <Prelude/HashTable.hpp>

namespace Mirb
{
	class Global:
		public Value::Header
	{
		public:
			typedef value_t (*reader_t)(Global *global);
			typedef void (*writer_t)(Global *global, value_t value);

			reader_t reader;
			writer_t writer;
			value_t value;
			
			Global() : Value::Header(Value::InternalGlobal), reader(0), writer(0), value(value_nil) {}

			void set(value_t new_value)
			{
				if(writer)
					writer(this, new_value);
				else
					value = new_value;
			}
			
			value_t get()
			{
				if(reader)
					return reader(this);
				else
					return value;
			}

			template<typename F> void mark(F mark)
			{
				mark(value);
			}
	};
};
