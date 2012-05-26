#pragma once
#include "value.hpp"
#include <Prelude/HashTable.hpp>

namespace Mirb
{
	class Global:
		public Value::Header
	{
		public:
			typedef value_t (*reader_t)(Global *global, Symbol *name);
			typedef bool (*writer_t)(Global *global, Symbol *name, value_t value);

			reader_t reader;
			writer_t writer;
			value_t value;
			
			Global() : Value::Header(Value::InternalGlobal), reader(0), writer(0), value(value_nil) {}

			static bool read_only_global(Global *global, Symbol *name, value_t value);

			void read_only()
			{
				writer = read_only_global;
			}

			bool set(Symbol *name, value_t new_value)
			{
				if(writer)
					return writer(this, name, new_value);
				else
				{
					value = new_value;
					return true;
				}
			}
			
			value_t get(Symbol *name)
			{
				if(reader)
					return reader(this, name);
				else
					return value;
			}

			template<typename F> void mark(F mark)
			{
				mark(value);
			}
	};
};
