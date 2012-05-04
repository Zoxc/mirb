#include "platform.hpp"
#include "../collector.hpp"
#include <signal.h>
#include <unistd.h>

#ifndef WIN32

namespace Mirb
{
	namespace Platform
	{
		std::string BenchmarkResult::format()
		{
			std::stringstream result;

			result << ((double)time / 1000) << " ms";

			return result.str();
		}

		void *allocate_region(size_t bytes)
		{
			void *result = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

			mirb_runtime_assert(result != 0);

			return result;
		}
		
		void free_region(void *region, size_t bytes)
		{
			munmap(region, bytes);
		}
		
		CharArray cwd()
		{
			char *result = getcwd(nullptr, 0);

			CharArray str((const char_t *)result, std::strlen(result));

			std::free(result);

			return str;
		}
		
		void signal_handler(int)
		{
			Collector::signal();
		}

		void initialize()
		{
			signal(SIGINT, signal_handler);
		}
	};
};

#endif
