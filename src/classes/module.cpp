#include "class.hpp"

namespace Mirb
{
	value_t Module::class_ref;

	BlockMap *Module::get_methods()
	{
		if(!methods)
			methods = new BlockMap(methods_initial);
		
		return methods;
	}
};

