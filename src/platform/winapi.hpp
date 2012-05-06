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
		
		template<Color input> void color(const CharArray &string)
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
					
				case Purple:
					flags = FOREGROUND_RED | FOREGROUND_BLUE;
					break;
					
				case Blue:
					flags = FOREGROUND_BLUE;
					break;
					
				case Green:
					flags = FOREGROUND_GREEN;
					break;
					
				case Gray:
					flags = FOREGROUND_INTENSITY;
					break;

				default:
					std::cerr << string.get_string();
					return;
			};

			SetConsoleTextAttribute(console, flags | (info.wAttributes & ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)));
			
			std::cerr << std::flush << string.get_string();

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
