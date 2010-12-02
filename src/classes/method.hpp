#pragma once
#include "object.hpp"

namespace Mirb
{
	class Method:
		public Object
	{
		public:
			// Used as DSL for native methods
			enum Flags
			{
				Internal,
				Static,
				Singleton
			};
	};
};
