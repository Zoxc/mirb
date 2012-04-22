#include "collector.hpp"

namespace Mirb
{
	FastList<ObjectHeader, ObjectHeader, &ObjectHeader::header_entry> Collector::object_list;
};
