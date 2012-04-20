#include "collector.hpp"

namespace Mirb
{
	SimplerList<ObjectHeader, ObjectHeader, &ObjectHeader::header_entry> Collector::object_list;
};
