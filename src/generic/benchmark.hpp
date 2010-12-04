#pragma once
#include "../common.hpp"

#ifndef WIN32
	#include <sys/time.h>
#endif

namespace Mirb
{
	class BenchmarkResult
	{
		public:
			#ifdef WIN32
				LARGE_INTEGER time, freq;
			#else
				uint64_t time; 
			#endif

			std::string format();
	};

	template<typename T> BenchmarkResult benchmark(T work)
	{
		BenchmarkResult result;

		#ifdef WIN32
			LARGE_INTEGER start, stop;

			QueryPerformanceFrequency(&result.freq);
			QueryPerformanceCounter(&start);

			work();

			QueryPerformanceCounter(&stop);

			result.time.QuadPart = stop.QuadPart - start.QuadPart;
		#else
			timeval start, stop;

			gettimeofday(&start, 0); 

			work();

			gettimeofday(&stop, 0); 

			result.time = ((uint64_t)stop.tv_sec * 1000000 + stop.tv_usec) - ((uint64_t)start.tv_sec * 1000000 + start.tv_usec);
		#endif

		return result;
	};
};
