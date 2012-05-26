#include "common.hpp"
#include "compiler.hpp"
#include "runtime.hpp"
#include "parser/parser.hpp"
#include "classes/exception.hpp"
#include "classes/string.hpp"
#include "classes/array.hpp"
#include "classes/fixnum.hpp"
#include "modules/kernel.hpp"
#include "platform/platform.hpp"
#include "block.hpp"
#include "document.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "tree/printer.hpp"
#endif

using namespace Mirb;

void report_exception()
{
	Exception *exception = context->exception;

	swallow_exception();

	OnStack<1> os(exception);

	Platform::color<Platform::Red>(inspect_obj(real_class_of(exception)));
			
	Platform::color<Platform::Bold>(": " + exception->message->string.get_string() + "\n");

	StackFrame::print_backtrace(exception->backtrace);
}

int main(int argc, const char *argv[])
{
	Mirb::initialize(true);
	
	if(argc > 1)
	{
		int index = 1;

		if(strcmp(argv[index], "-v") == 0)
		{
			std::cout << "mirb 0.1" << std::endl;
			index++;
		}
		
		if(strcmp(argv[index], "--syntax") == 0)
		{
			int result = 0;

			for(int i = ++index; i < argc; ++i)
			{
				MemoryPool::Base memory_pool;
				CharArray filename = (const char_t *)argv[i];
				
				char_t *data;
				size_t length;
				bool loaded;
				CharArray full_path;

				if(!Kernel::read_file(filename, true, false, full_path, loaded, data, length))
				{
					report_exception();
					return 1;
				}

				Document *document = Collector::allocate_pinned<Document>();

				document->data = (const char_t *)data;
				document->length = length;
				document->name = full_path;

				Parser parser(symbol_pool, memory_pool, document);

				parser.load();
				parser.parse_main();

				if(!parser.messages.empty())
					result = 1;
		
				for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
					i().print();
			}

			return result;
		}

		CharArray exec = CharArray((const char_t *)argv[index++]);
		
		Array *new_argv = Collector::allocate<Array>();

		for(int i = index; i < argc; ++i)
		{
			new_argv->vector.push(CharArray((const char_t *)argv[i]).to_string());
		}

		set_const(context->object_class, Symbol::get("ARGV"), new_argv);

		if(!Kernel::load(exec.to_string()))
		{
			report_exception();
			return 1;
		}

		return 0;
	}
	
	while(1)
	{
		mirb_debug(Collector::collect());

		std::string line;
		
		std::getline(std::cin, line);

		Block *block;

		{
			MemoryPool::Base memory_pool;
			Document *document = Collector::allocate_pinned<Document>();

			document->copy((const char_t *)line.c_str(), line.length());
		
			document->name = "Input";

			Parser parser(symbol_pool, memory_pool, document);

			parser.load();
		
			if(parser.lexeme() == Lexeme::END && parser.messages.empty())
				break;
		
			Tree::Scope *scope = parser.parse_main();
		
			if(!parser.messages.empty())
			{
				for(auto i = parser.messages.begin(); i != parser.messages.end(); ++i)
					i().print();
				
				continue;
			}
	
			#ifdef MIRB_DEBUG_COMPILER
				DebugPrinter printer;
			
				std::cout << "Parsing done.\n-----\n";
				std::cout << printer.print_node(scope->group);
				std::cout << "\n-----\n";
			#endif
		
			block = Compiler::compile(scope, memory_pool);
		}

		value_t result = call_code(block, context->main, Symbol::get("main"), context->object_scope, value_nil, 0, 0);

		try
		{
			if(result == value_raise)
			{
				report_exception();
				std::cerr << "\n";
			}
			else
				std::cout << "=> " << inspect_object(result) << "\n";
		} catch(Exception *e)
		{
			if(real_class_of(e) == context->interrupt_class)
				std::cout << "Interrupted!\n";
			else
				std::cout << "Unable to inspect result" << "\n";
		}
	}
	
	std::cout << std::endl << "Number of collections: " << Collector::collections << std::endl;
	std::cout << "Memory allocated: " << (Collector::memory / 1024) <<  " KiB" << std::endl;
	std::cout << "Regions allocated: " << Collector::region_count << std::endl;
	std::cout << "Regions freed: " << Collector::region_free_count << std::endl;
	std::cout << "Exiting gracefully..." << std::endl;
	
	Mirb::finalize();
	
	return 0;
}
