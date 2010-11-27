#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "object.hpp"

namespace Mirb
{
	class Class:
		public Object
	{
		private:
			BlockMap *methods;

		public:
			value_t superclass;

			static const size_t methods_initial = 1;
			
			BlockMap *get_methods();

			static value_t class_ref;
	};
};

