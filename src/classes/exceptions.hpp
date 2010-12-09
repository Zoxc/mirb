#pragma once
#include "exception.hpp"

namespace Mirb
{
	class StandardError:
		public Exception
	{
		public:
			StandardError(value_t message, value_t backtrace) : Exception(message, backtrace) {}
			
			static value_t class_ref;

			static void initialize();
	};

	class NameError:
		public StandardError
	{
		public:
			NameError(value_t message, value_t backtrace) : StandardError(message, backtrace) {}
			
			static value_t class_ref;

			static void initialize();
	};
};
