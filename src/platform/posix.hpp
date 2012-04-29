namespace Mirb
{
	namespace Platform
	{
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
