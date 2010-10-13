#include "compiler.hpp"

namespace Mirb
{
	Compiler::Compiler()
	{
	}

	Compiler::~Compiler()
	{
	}
	
	void Compiler::report(Range &range, std::string text, Message::Severity severity)
	{
		new StringMessage(*this, range, severity, text);
	}
};
