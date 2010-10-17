#include "globals.hpp"
#include "compiler/lexer.hpp"
#include "runtime/classes.hpp"
#include "runtime/classes/symbol.hpp"
#include "runtime/classes/string.hpp"
#include "compiler/generator/generator.hpp"
#include "compiler/generator/x86.hpp"

#include <iostream>
#include "src/compiler.hpp"

using namespace Mirb;

int main()
{
	#ifndef VALGRIND
		GC_INIT();
	#endif
	
	rt_create();
	
	while(1)
	{
		std::string line;
		
		std::getline(std::cin, line);
		
		Compiler compiler;
		
		compiler.filename = "Input";
		compiler.load((const char_t *)line.c_str(), line.length());
		
		if(compiler.parser.lexeme() == Lexeme::END)
			break;
		
		Scope scope;
		
		compiler.parser.parse_main(scope);
		
		struct node* expression = 0;//TODO: update - compiler.parser.parse_main();
		
		if(compiler.messages.empty())
		{
			#ifdef DEBUG
				printf("Parsing done.\n");
				printf("Tree: %s\n", rt_string_to_cstr(get_node_name(expression)));
			#endif
			
			struct block *block = gen_block(expression);
			
			struct rt_block *runtime_block = compile_block(block);
			
			#ifdef DEBUG
				printf("Running block %x: ", (rt_value)runtime_block);
			#endif
			
			//__asm__("int3\n"); // Make debugging life easier
			
			rt_value result = runtime_block->compiled(0, RT_NIL, rt_class_of(rt_main), rt_main, RT_NIL, 0, 0);
			
			printf("=> "); rt_print(result); printf("\n");
			
			runtime_block = 0; // Make sure runtime_block stays on stack
		}
		else
		{
			for(auto i = compiler.messages.begin(); i; ++i)
				std::cout << i().format() << "\n";
		}
	}
	
	printf("Exiting gracefully...");
	
	rt_destroy();
	
	return 0;
}
