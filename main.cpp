#include "globals.hpp"
#include "runtime/classes.hpp"
#include "runtime/classes/symbol.hpp"
#include "runtime/classes/string.hpp"

#include <iostream>
#include "src/compiler.hpp"
#include "src/runtime.hpp"
#include "src/parser/parser.hpp"
#include "src/block.hpp"

#ifdef DEBUG
	#include "src/tree/printer.hpp"
#endif

using namespace Mirb;

int main()
{
	#ifndef NO_GC
		GC_INIT();
	#endif
	
	rt_create();
	Mirb::initialize();
	
	while(1)
	{
		std::string line;
		
		std::getline(std::cin, line);
		
		CharArray filename("Input");
		
		MemoryPool memory_pool;
		Parser parser(symbol_pool, memory_pool, filename);
		
		parser.load((const char_t *)line.c_str(), line.length());
		
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
	
		#ifdef DEBUG
			DebugPrinter printer;
			
			std::cout << "Parsing done.\n-----\n";
			std::cout << printer.print_node(scope->group);
			std::cout << "\n-----\n";
		#endif
		
		Block *block = Compiler::compile(scope, memory_pool);

		rt_value result = block->compiled(value_nil, rt_class_of(rt_main), rt_main, value_nil, 0, 0);
		
		printf("=> "); rt_print(result); printf("\n");
		
		block = 0; // Make sure block stays on stack */
	}
	
	std::cout << "Exiting gracefully...";
	
	Mirb::finalize();
	rt_destroy();
	
	return 0;
}
