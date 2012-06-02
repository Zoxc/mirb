#include "common.hpp"
#include "compiler.hpp"
#include "runtime.hpp"
#include "parser/parser.hpp"
#include "classes/exception.hpp"
#include "classes/string.hpp"
#include "classes/array.hpp"
#include "classes/fixnum.hpp"
#include "classes/file.hpp"
#include "classes/float.hpp"
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
		value_t io = context->io_err;
		
		context->console_error->print("\n");

		call(exception, "print", io);
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

int run_file(int index, int argc, const char *argv[])
{
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
		auto system_exit = try_cast<SystemExit>(exception);

		if(system_exit)
			return system_exit->result;

		report_exception(exception);

		return 1;
	}
	else
		return 0;
}

void print_stats()
{
	Exception *exception;

	trap_exception(exception, [&] {
		context->console_output->puts("\nTime in GC: " + inspect(Float::allocate(Collector::bench.time())) + " ms");
		context->console_output->puts("\nAverage time per GC: " + inspect(Float::allocate(Collector::bench.time() / Collector::collections)) + " ms");
		context->console_output->puts("Number of collections: " + CharArray::uint(Collector::collections));
		context->console_output->puts("Memory allocated: " + CharArray::uint((size_t)((Collector::memory + Collector::total_memory) / 1024)) + " KiB");
		context->console_output->puts("Regions allocated: " + CharArray::uint(Collector::region_count));
		context->console_output->puts("Regions freed: " + CharArray::uint(Collector::region_free_count));
	});
}

int main(int argc, const char *argv[])
{
	Mirb::initialize(true);
	
	if(argc > 1)
	{
		int index = 1;

		if(strcmp(argv[index], "-v") == 0)
		{
			context->console_output->puts("mirb 0.1");
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
		
		Platform::BenchmarkResult bench;
		
		int result;

		Platform::benchmark(bench, [&] {
			result = run_file(index, argc, argv);
		});

		Exception *exception;

		trap_exception(exception, [&] {
			context->console_output->puts("\nTime overall: " + inspect(Float::allocate(bench.time())) + " ms");
		});

		print_stats();

		Mirb::finalize();

		return result;
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
				trap_exception(exception, [&] { context->console_output->puts("=> " + inspect(result)); });

				if(exception)
				{
					context->console_error->puts("Unable to inspect result:");
					report_exception(exception);
				}
			}
		});

		if(exception)
		{
			auto system_exit = try_cast<SystemExit>(exception);

			if(system_exit)
				context->console_error->puts("Code exited with value: " + Number((intptr_t)system_exit->result).to_string());
			else
				report_exception(exception);
		}
	}
	
	print_stats();

	Mirb::finalize();
	
	return 0;
}
