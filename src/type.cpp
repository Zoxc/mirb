#include "type.hpp"

namespace Mirb
{
	std::string Type::names[] = {
		"None",
		"FreeBlock",
		"InternalValueMap",
		"InternalStackFrame",
		"InternalTuple",
		"InternalValueTuple",
		"InternalVariableBlock",
		"InternalDocument",
		"InternalBlock",
		"InternalScope",
		"InternalGlobal",
		"Float",
		"Range",
		"Fixnum",
		"TrueClass",
		"FalseClass",
		"NilClass",
		"Class",
		"IClass",
		"Module",
		"Object",
		"Symbol",
		"String",
		"Regexp",
		"Bignum",
		"Array",
		"Hash",
		"Method",
		"Proc",
		"Exception",
		"ReturnException",
		"BreakException",
		"NextException",
		"RedoException",
		"SystemStackError",
		"SyntaxError",
		"IO"
	};
};
