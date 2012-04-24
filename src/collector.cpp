#include "collector.hpp"

namespace Mirb
{
	const size_t header_magic = 12345;

	BasicObjectHeader::BasicObjectHeader(Value::Type type) : type(type)
	{
		#ifdef DEBUG
			magic = header_magic;
		#endif
	}

	Value::Type BasicObjectHeader::get_type()
	{
		return type;
	}
	
	bool inverted;
	
	void Collector::collect()
	{
	}

	size_t Collector::allocated;
	FastList<ObjectHeader, BasicObjectHeader, &BasicObjectHeader::header_entry> Collector::object_list;
	FastList<PinnedHeader, BasicObjectHeader, &BasicObjectHeader::header_entry> Collector::pinned_object_list;
};
