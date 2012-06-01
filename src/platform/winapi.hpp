#pragma once
#include <tchar.h>

namespace Mirb
{
	namespace Platform
	{
		class BenchmarkResult
		{
			public:
				LARGE_INTEGER time, freq;

				CharArray format();
		};
		
		void raise(const CharArray &message);

		class NativeStream:
			public Stream
		{
			protected:
				HANDLE handle;

			public:
				virtual void print(const CharArray &string);
				virtual pos_t pos();
				virtual pos_t size();
				virtual CharArray read(size_t length);
				virtual void seek(pos_t val, PosType type);

				NativeStream(HANDLE handle);
				~NativeStream();
		};
		
		class ConsoleStream:
			public NativeStream
		{
			public:
				virtual void color(Color color, const CharArray &string);

				ConsoleStream(HANDLE handle) : NativeStream(handle) {}
		};

		CharArray to_win_path(const CharArray &path);
		CharArray from_tchar(const TCHAR *buffer, size_t tchars);
		CharArray from_tchar(const TCHAR *buffer);
		CharArray normalize_separator_to(const CharArray &path);

		template<typename F> void to_tchar(const CharArray &string, F func)
		{
			#ifdef _UNICODE
				mirb_runtime_assert(string.size() < INT_MAX);

				int size = MultiByteToWideChar(CP_UTF8, 0, (const char *)string.raw(), (int)string.size(), nullptr, 0);
				
				if(size < 0)
					raise("Unable to convert UTF-8 pathname to UCS-2");

				TCHAR *buffer = (TCHAR *)alloca((size + 1) * sizeof(TCHAR));

				size = MultiByteToWideChar(CP_UTF8, 0, (const char *)string.raw(), (int)string.size(), buffer, size);

				if(size <= 0)
					raise("Unable to convert UTF-8 pathname to UCS-2");

				buffer[size] = 0;

				func(buffer, size);
			#else
				func((TCHAR *)string.raw(), string.size());
			#endif
		}
		
		template<typename F> void to_short_win_path(const CharArray &path, F func)
		{
			CharArray input = normalize_separator_to(native_path(path));

			#ifdef _UNICODE
				if(input.size() > MAX_PATH)
				{
					to_tchar("\\\\?\\" + input, [&](TCHAR *buffer, size_t size prelude_unused) {
						mirb_runtime_assert(size < UINT_MAX);

						if(GetShortPathName(buffer, buffer, (DWORD)size) == 0)
							raise("Unable to convert long pathname to a short pathname");

						func(buffer);
					});
				}
			#endif

			to_tchar(input, [&](TCHAR *buffer, size_t size prelude_unused) {
				func(buffer);
			});
		}
	
		template<typename F> void list_dir(const CharArray &path, bool skip_special, F func)
		{
			WIN32_FIND_DATA ffd;
			HANDLE handle;

			to_tchar(to_win_path(path) + "\\*", [&](const TCHAR *buffer, size_t) {
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
