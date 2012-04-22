#pragma once
#include <Prelude/List.hpp>
#include "value.hpp"

namespace Mirb
{
	class ObjectHeader
	{
		private:
			#ifdef DEBUG
				size_t magic;
			#endif

			const Value::Type type;

			bool value;
		public:
			ListEntry<ObjectHeader> header_entry;

			ObjectHeader(Value::Type type);
			
			Value::Type get_type();
	};

	template<Value::Type class_type> class ConstantHeader:
		public ObjectHeader
	{
		public:
			ConstantHeader() : ObjectHeader(class_type) {}
	};
};

