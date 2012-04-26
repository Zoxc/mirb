#include "common.hpp"
#include "compiler.hpp"
#include "runtime.hpp"
#include "parser/parser.hpp"
#include "classes/exception.hpp"
#include "classes/string.hpp"
#include "block.hpp"
#include "document.hpp"
#include <Prelude/Region.hpp>

#ifdef DEBUG
	#include "tree/printer.hpp"
#endif

using namespace Mirb;

int main()
{
	#ifndef NO_GC
		GC_INIT();
	#endif

	Mirb::initialize();
	
	while(1)
	{
		Collector::collect();

		std::string line;
		
		std::getline(std::cin, line);
		
		MemoryPool memory_pool;
		Document *document = Collector::allocate<Document>();

		document->copy((const char_t *)line.c_str(), line.length());
		
		document->name = "Input";

		Parser parser(symbol_pool, memory_pool, document);

		parser.load();
		
		if(parser.lexeme() == Lexeme::END && parser.messages.empty())
			break;
		
		Tree::Fragment fragment(0, Tree::Chunk::main_size);
		Tree::Scope *scope = parser.parse_main(&fragment);
		
		if(!parser.messages.empty())
		{
			for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
				std::cout << i().format() << "\n";
				
			continue;
		}
	
		#ifdef MIRB_DEBUG_COMPILER
			DebugPrinter printer;
			
			std::cout << "Parsing done.\n-----\n";
			std::cout << printer.print_node(scope->group);
			std::cout << "\n-----\n";
		#endif
		
		Block *block = Compiler::compile(scope, memory_pool);
		
		value_t result = call_code(block, context->main, Symbol::from_literal("main"), class_of(context->main), value_nil, 0, 0);
		
		OnStack<1> os1(result);

		if(result == value_raise)
		{
			Exception *exception = current_exception;

			OnStack<1> os2(exception);

			std::cout << inspect_object(real_class_of(auto_cast(exception))) << ": " << enforce_string(exception->message)->string.get_string() << "\n" << StackFrame::get_backtrace(exception->backtrace).get_string() << "\n";
		}
		else
			std::cout << "=> " << inspect_object(result) << "\n";
	}
	
	std::cout << "Exiting gracefully...";
	
	Mirb::finalize();
	
	return 0;
}
