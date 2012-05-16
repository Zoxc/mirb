#include "../classes/file.hpp"
#include <tchar.h>

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
		
		CharArray convert_path(const CharArray &path);
		CharArray from_tchar(const TCHAR *buffer, size_t tchars);
		CharArray from_tchar(const TCHAR *buffer);

		template<typename F> void to_tchar(const CharArray &string, F func)
		{
			#ifdef _UNICODE
				size_t size = MultiByteToWideChar(CP_UTF8, 0, (const char *)string.raw(), string.size(), nullptr, 0);

				TCHAR *buffer = (TCHAR *)alloca((size + 1) * sizeof(TCHAR));

				size = MultiByteToWideChar(CP_UTF8, 0, (const char *)string.raw(), string.size(), buffer, size);

				buffer[size] = 0;

				func(buffer, size);
			#else
				func((const TCHAR *)string.raw(), string.size());
			#endif
		}

		template<typename F> bool list_dir(const CharArray &path, bool skip_special, F func)
		{
			WIN32_FIND_DATA ffd;
			HANDLE handle;

			to_tchar(convert_path(File::join(File::expand_path(path), '*')), [&](const TCHAR *buffer, size_t) {
				handle = FindFirstFile(buffer, &ffd);
			});

			if(handle == INVALID_HANDLE_VALUE)
				return false;

			do
			{
				if(skip_special && (_tcscmp(ffd.cFileName, _T("..")) == 0 || _tcscmp(ffd.cFileName, _T(".")) == 0))
					continue;

				func(from_tchar(ffd.cFileName), (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
			} while (FindNextFile(handle, &ffd) != 0);

			FindClose(handle);

			return true;
		}

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
