#include <sys/time.h>

namespace Mirb
{
	namespace Platform
	{
		template<Color input> void color(const CharArray &string)
		{
			switch(input)
			{
				case Red:
					std::cerr << ("\033[1;31m" + string + "\033[0m").get_string();
					break;

				case Blue:
					std::cerr << ("\033[1;34m" + string + "\033[0m").get_string();
					break;

				case Green:
					std::cerr << ("\033[1;32m" + string + "\033[0m").get_string();
					break;

				case Bold:
					std::cerr << ("\033[1m" + string + "\033[0m").get_string();
					break;

				default:
					std::cerr << string.get_string();
					return;
			};
		};

		class BenchmarkResult
		{
			public:
				uint64_t time; 

				std::string format();
		};

		template<typename T> BenchmarkResult benchmark(T work)
		{
			BenchmarkResult result;

			timeval start, stop;

			gettimeofday(&start, 0); 

			work();

			gettimeofday(&stop, 0); 

			result.time = ((uint64_t)stop.tv_sec * 1000000 + stop.tv_usec) - ((uint64_t)start.tv_sec * 1000000 + start.tv_usec);
			
			return result;
		};
	};
};
