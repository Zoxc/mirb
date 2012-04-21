#include "object-header.hpp"

namespace Mirb
{
	const size_t header_magic = 12345;

	ObjectHeader::ObjectHeader(Value::Type type) : type(type), value(false)
	{
		#ifdef DEBUG
			magic = header_magic;
		#endif
	}

	Value::Type ObjectHeader::get_type()
	{
		return type;
	}
};

