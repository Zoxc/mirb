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
		
		void raise(const CharArray &message);

		CharArray to_win_path(const CharArray &path);
		CharArray from_tchar(const TCHAR *buffer, size_t tchars);
		CharArray from_tchar(const TCHAR *buffer);

		template<typename F> void to_tchar(const CharArray &string, F func)
		{
			#ifdef _UNICODE
				size_t size = MultiByteToWideChar(CP_UTF8, 0, (const char *)string.raw(), string.size(), nullptr, 0);

				TCHAR *buffer = (TCHAR *)alloca((size + 1) * sizeof(TCHAR));

				size = MultiByteToWideChar(CP_UTF8, 0, (const char *)string.raw(), string.size(), buffer, size);

				if(size == 0)
					raise("Unable to convert UTF-8 pathname to UCS-2");

				buffer[size] = 0;

				func(buffer, size);
			#else
				func((TCHAR *)string.raw(), string.size());
			#endif
		}
		
		template<typename F> void to_short_win_path(const CharArray &path, F func)
		{
			to_tchar(to_win_path(expand_path(path)), [&](TCHAR *buffer, size_t size) {
				#ifdef _UNICODE
					if(size > MAX_PATH)
					{
						if(GetShortPathName(buffer, buffer, size + 1) == 0)
							raise("Unable to convert long pathname to a short pathname");
					}
				#endif

				func(buffer);
			});
		}
	
		template<typename F> void list_dir(const CharArray &path, bool skip_special, F func)
		{
			WIN32_FIND_DATA ffd;
			HANDLE handle;

			if(path.size() == 0)
				throw create_exception(context->system_call_error, "Empty path");

			to_tchar(to_win_path(File::join(expand_path(path), '*')), [&](const TCHAR *buffer, size_t) {
				handle = FindFirstFile(buffer, &ffd);
			});

			if(handle == INVALID_HANDLE_VALUE)
				raise("Unable to list directory '" + path + "'");

			do
			{
				if(skip_special && (_tcscmp(ffd.cFileName, _T("..")) == 0 || _tcscmp(ffd.cFileName, _T(".")) == 0))
					continue;

				func(from_tchar(ffd.cFileName), (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
			} while (FindNextFile(handle, &ffd) != 0);

			FindClose(handle);
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
