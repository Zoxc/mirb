#pragma once
#include "value.hpp"
#include "generic/simpler-list.hpp"

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
			SimpleEntry<ObjectHeader> header_entry;

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

