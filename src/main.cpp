#include "common.hpp"
#include "compiler.hpp"
#include "runtime.hpp"
#include "parser/parser.hpp"
#include "classes/exception.hpp"
#include "classes/string.hpp"
#include "classes/array.hpp"
#include "classes/fixnum.hpp"
#include "classes/file.hpp"
#include "modules/kernel.hpp"
#include "platform/platform.hpp"
#include "block.hpp"
#include "document.hpp"
#include "number.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "tree/printer.hpp"
#endif

using namespace Mirb;

void report_exception(Exception *exception, bool recurse = true)
{
	OnStack<1> os(exception);

	trap_exception(exception, [&] {
		context->console_error->color(Red, inspect(class_of(exception)));
			
		context->console_error->print(": ");

		value_t io = new (collector) IO(context->console_error, context->io_class, false);

		call_argv(exception, "print", 1, &io);

		context->console_error->print("\n");

		StackFrame::print_backtrace(exception->backtrace, *context->console_error);
	});
	
	if(exception)
	{
		std::cerr << "Unable to inspect exception:\n";

		if(recurse)
			report_exception(exception, false);
	}

	std::cerr << "\n";
}

bool load_core_lib(const char *executable)
{
	CharArray path = File::join(File::dirname(File::expand_path((const char_t *)executable)), "corelib");

	Exception *exception;

	trap_exception(exception, [&] {
		Kernel::load(File::join(path, "core.rb").to_string());
	});

	if(exception)
	{
		report_exception(exception);
		return false;
	}

	return true;
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

				Exception *exception;

				trap_exception(exception, [&] { Kernel::read_file(filename, true, false, full_path, loaded, data, length); });

				if(exception)
				{
					report_exception(exception);
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
					i().print(*context->console_error);
			}

			return result;
		}

		load_core_lib(argv[0]);

		CharArray exec = CharArray((const char_t *)argv[index++]);
		
		Array *new_argv = Collector::allocate<Array>();

		for(int i = index; i < argc; ++i)
		{
			new_argv->vector.push(CharArray((const char_t *)argv[i]).to_string());
		}

		set_const(context->object_class, Symbol::get("ARGV"), new_argv);
		
		Exception *exception;

		trap_exception(exception, [&] { Kernel::load(exec.to_string()); });

		if(exception)
		{
			report_exception(exception);
			return 1;
		}

		return 0;
	}
	
	load_core_lib(argv[0]);

	set_const(context->object_class, Symbol::get("ARGV"), new (collector) Array);

	while(1)
	{
		mirb_debug(Collector::collect());

		std::string line;
		
		std::getline(std::cin, line);

		if(line == "")
			break;

		Exception *exception;

		trap_exception(exception, [&] { 
			value_t result = eval(context->main, Symbol::get("main"), context->object_scope, (const char_t *)line.c_str(), line.length(), "Input", false);

			if(result)
			{
				trap_exception(exception, [&] { std::cout << "=> " << inspect_object(result) << "\n"; });

				if(exception)
				{
					std::cerr << "Unable to inspect result:\n";
					report_exception(exception);
				}
			}
		});

		if(exception)
			report_exception(exception);
	}
	
	std::cout << std::endl << "Number of collections: " << Collector::collections << std::endl;
	std::cout << "Memory allocated: " << (Collector::memory / 1024) <<  " KiB" << std::endl;
	std::cout << "Regions allocated: " << Collector::region_count << std::endl;
	std::cout << "Regions freed: " << Collector::region_free_count << std::endl;
	std::cout << "Exiting gracefully..." << std::endl;
	
	Mirb::finalize();
	
	return 0;
}
