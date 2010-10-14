#include "compiler.hpp"
#include "symbol-pool.hpp"

namespace Mirb
{
	Compiler::Compiler() : parser(symbol_pool, memory_pool, *this), filename(0)
	{
	}

	Compiler::~Compiler()
	{
	}
	
	void Compiler::load(const char_t *input, size_t length)
	{
		parser.lexer.load(input, length);
	}
	
	void Compiler::report(Range &range, std::string text, Message::Severity severity)
	{
		new StringMessage(*this, range, severity, text);
	}
};
