namespace Mirb
{
	namespace Platform
	{
		class BenchmarkResult
		{
			public:
				LARGE_INTEGER time, freq;

				std::string format();
		};

		template<typename T> BenchmarkResult benchmark(T work)
		{
			BenchmarkResult result;

			LARGE_INTEGER start, stop;

			QueryPerformanceFrequency(&result.freq);
			QueryPerformanceCounter(&start);

			work();

			QueryPerformanceCounter(&stop);

			result.time.QuadPart = stop.QuadPart - start.QuadPart;

			return result;
		};
	};
};
