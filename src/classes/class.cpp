#include "class.hpp"

namespace Mirb
{
	value_t Class::class_ref;

	BlockMap *Class::get_methods()
	{
		if(!methods)
			methods = new BlockMap(methods_initial);
		
		return methods;
	}
};

