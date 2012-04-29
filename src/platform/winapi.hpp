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
		
		template<Color input, typename T> void color(T func)
		{
			WORD flags;
			
			HANDLE console = GetStdHandle(STD_ERROR_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO info;
			
			GetConsoleScreenBufferInfo(console, &info);
			
			switch(input)
			{
				case Red:
					flags = FOREGROUND_RED;
					break;
					
				case Green:
					flags = FOREGROUND_GREEN;
					break;
					
				case Gray:
					flags = FOREGROUND_INTENSITY;
					break;

				default:
					func();
					return;
			};

			SetConsoleTextAttribute(console, flags | (info.wAttributes & ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)));
			
			std::cerr << std::flush;

			func();
			
			SetConsoleTextAttribute(console, info.wAttributes);
			
			std::cerr << std::flush;
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
