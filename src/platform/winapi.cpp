#include "platform.hpp"

#ifdef WIN32

#include "../classes/string.hpp"
#include "../collector.hpp"
#include "../runtime.hpp"
#include <intrin.h>

namespace Mirb
{
	namespace Platform
	{
		void raise(const CharArray &message)
		{
			TCHAR *msg_buffer;
			DWORD err_no = GetLastError(); 

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |	FORMAT_MESSAGE_IGNORE_INSERTS, 0, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg_buffer, 0, 0);

			CharArray msg = message + "\nError #" + CharArray::uint(err_no) + ": "   + from_tchar(msg_buffer);

			LocalFree(msg_buffer);
			
			raise(context->system_call_error, msg);
		}
		
		double get_time()
		{
			SYSTEMTIME system_time;

			GetSystemTime(&system_time);
			
			FILETIME file_time;

			SystemTimeToFileTime(&system_time, &file_time);

			ULARGE_INTEGER large_time;

			large_time.LowPart = file_time.dwLowDateTime;
			large_time.HighPart = file_time.dwHighDateTime;

			return (double)(large_time.QuadPart - 116444736000000000) / 10000000;
		}

		size_t stack_start()
		{
			size_t offset = offsetof(NT_TIB, StackBase);

			#ifdef _AMD64_
				return (size_t)__readgsqword((DWORD)offset);
			#else
				return (size_t)__readfsdword((DWORD)offset);
			#endif
		}

		size_t stack_limit()
		{
			auto module = GetModuleHandle(0);

			mirb_runtime_assert(module != INVALID_HANDLE_VALUE);

			auto dos_header = (IMAGE_DOS_HEADER *)module;

			auto nt_headers = (IMAGE_NT_HEADERS *)((char *)dos_header + dos_header->e_lfanew);

			return nt_headers->OptionalHeader.SizeOfStackReserve;
		}
		
		std::string BenchmarkResult::format()
		{
			std::stringstream result;

			result << (((double)1000 * time.QuadPart) / (double)freq.QuadPart) << " ms";

			return result.str();
		}

		void *allocate_region(size_t bytes)
		{
			void *result = VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			mirb_runtime_assert(result != 0);

			return result;
		}
	
		void free_region(void *region, size_t)
		{
			VirtualFree(region, 0, MEM_RELEASE);
		}
		
		CharArray from_tchar(const TCHAR *buffer, size_t tchars)
		{
			#ifdef _UNICODE
				mirb_runtime_assert(tchars < INT_MAX);

				int size = WideCharToMultiByte(CP_UTF8, 0, buffer, (int)tchars, nullptr, 0, NULL, NULL);

				char *new_buffer = (char *)alloca(size);

				size = WideCharToMultiByte(CP_UTF8, 0, buffer, (int)tchars, new_buffer, size, NULL, NULL);

				if(size == 0)
					raise("Unable to convert UCS-2 pathname to UTF-8");

				return CharArray((const char_t *)new_buffer, size);
			#else
				return CharArray((const char_t *)buffer, tchars);
			#endif
		}
		
		CharArray from_tchar(const TCHAR *buffer)
		{
			return from_tchar(buffer, _tcslen(buffer));
		}

		BOOL WINAPI ctrl_handler(DWORD ctrl_type) 
		{ 
			switch(ctrl_type) 
			{ 
				case CTRL_C_EVENT: 
					Collector::signal();
					return TRUE; 
 
				default: 
					return FALSE; 
			} 
		} 
 
		CharArray normalize_separator_to(const CharArray &path)
		{
			CharArray result = path;

			result.localize();

			for(size_t i = 0; i < result.size(); ++i)
				if(result[i] == '/')
					result[i] = '\\';

			if(result[result.size()-1] == '\\')
				result.shrink(result.size() - 1);

			return result;
		}
	
		CharArray normalize_separator_from(const CharArray &path)
		{
			CharArray result = path;

			result.localize();

			for(size_t i = 0; i < result.size(); ++i)
				if(result[i] == '\\')
					result[i] = '/';

			if(result[result.size()-1] == '/')
				result.shrink(result.size() - 1);

			return result;
		}
	
		CharArray from_win_path(const CharArray &path)
		{
			#ifdef _UNICODE
				CharArray result = (path.copy(0, 4) == "\\\\?\\") ? path.copy(4, path.size()) : path;

				return ruby_path(normalize_separator_from(result));
			#else
				return ruby_path(normalize_separator_from(path));
			#endif
		}
	
		CharArray to_win_path(const CharArray &path)
		{
			#ifdef _UNICODE
				return "\\\\?\\" + normalize_separator_to(native_path(path));
			#else
				return normalize_separator_to(native_path(path));
			#endif
		}
	
		CharArray from_short_win_path(const TCHAR *buffer)
		{
			#ifdef _UNICODE
				DWORD size = GetLongPathName(buffer, 0, 0);

				TCHAR *long_buffer = (TCHAR *)alloca((size + 1) * sizeof(TCHAR));

				if(GetLongPathName(buffer, long_buffer, size + 1) == 0)
					raise("Unable to convert short pathname to a long pathname");

				return ruby_path(normalize_separator_from(from_tchar(long_buffer)));
			#else
				return ruby_path(normalize_separator_from(from_tchar(buffer)));
			#endif
		}
	
		DWORD attributes(const CharArray &path)
		{
			DWORD attrib; 
			
			try
			{
				to_tchar(to_win_path(path), [&](const TCHAR *buffer, size_t) {
					attrib = GetFileAttributes(buffer);
				});
			}
			catch(empty_path_error)
			{
				attrib = INVALID_FILE_ATTRIBUTES;
			}

			return attrib;
		}
		
		CharArray cwd()
		{
			DWORD size = GetCurrentDirectory(0, 0);

			TCHAR *buffer = (TCHAR *)alloca(size * sizeof(TCHAR));

			if(GetCurrentDirectory(size, buffer) == 0)
				raise("Unable to get the current directory");

			return from_short_win_path(buffer);
		}

		void mkdir(const CharArray &path)
		{
			to_tchar(to_win_path(path), [&](const TCHAR *buffer, size_t) {
				if(CreateDirectory(buffer, 0) == 0)
					raise("Unable to create directory '" + path + "'");
			});
		}
		
		bool is_executable(const CharArray &) throw()
		{
			return false;
		}

		bool file_exists(const CharArray &path) throw()
		{
			return (attributes(path) != INVALID_FILE_ATTRIBUTES);
		}
		
		bool is_directory(const CharArray &path) throw()
		{
			DWORD attrib = attributes(path); 

			return ((attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		bool is_file(const CharArray &path) throw()
		{
			DWORD attrib = attributes(path); 

			return ((attrib != INVALID_FILE_ATTRIBUTES) && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		NativeStream *open(const CharArray &path, size_t access, Mode mode)
		{
			HANDLE result;

			to_tchar(to_win_path(path), [&](const TCHAR *buffer, size_t) {
				DWORD __access = 0;
				DWORD share = FILE_SHARE_READ;
				DWORD create;

				switch(mode)
				{
					case Open:
						create = OPEN_EXISTING;
						break;
					
					case CreateTruncate:
						create = CREATE_ALWAYS;
						break;

					case CreateAppend:
						create = OPEN_ALWAYS;
						break;
				}

				if(access & Read)
					__access |= GENERIC_READ;

				if(access & Write)
				{
					__access |= GENERIC_WRITE;
					share &= ~FILE_SHARE_READ;
				}

				result = CreateFile(buffer, __access, share, 0, create, FILE_ATTRIBUTE_NORMAL, 0);

				if(result == INVALID_HANDLE_VALUE)
					raise("Unable to open file '" + path + "'");

				if(mode == CreateAppend)
				{
					if(SetFilePointer(result, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER)
					{
						CloseHandle(result);

						raise("Unable to seek to end of file '" + path + "'");
					}
				}
			});

			return new NativeStream(result);
		}
		
		ConsoleStream *console_stream(ConsoleStreamType type)
		{
			HANDLE result;

			switch(type)
			{
				case StandardInput:
					result = GetStdHandle(STD_INPUT_HANDLE);
					break;

				case StandardOutput:
					result = GetStdHandle(STD_OUTPUT_HANDLE);
					break;

				case StandardError:
					result = GetStdHandle(STD_ERROR_HANDLE);
					break;
			}

			if(result == INVALID_HANDLE_VALUE)
				raise("Unable to get console stream");

			return new ConsoleStream(result);
		}
		
		NativeStream::NativeStream(HANDLE handle) : handle(handle)
		{
		}

		NativeStream::~NativeStream()
		{
			CloseHandle(handle);
		}
		
		void NativeStream::print(const CharArray &string)
		{
			DWORD out;
			if(!WriteFile(handle, string.str_ref(), string.size(), &out, 0))
				raise("Unable to write to handle");
		}
		
		void ConsoleStream::color(Color color, const CharArray &string)
		{
			CONSOLE_SCREEN_BUFFER_INFO info;
			WORD flags;
			
			GetConsoleScreenBufferInfo(handle, &info);
			
			switch(color)
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
					print(string);
					return;
			};

			SetConsoleTextAttribute(handle, flags | (info.wAttributes & ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)));

			Finally finally([&] {
				SetConsoleTextAttribute(handle, info.wAttributes);
			});
			
			print(string);
		}

		void initialize(bool console)
		{
			if(console)
			{
				SetConsoleTitle(TEXT("mirb"));

				if(!SetConsoleCtrlHandler(ctrl_handler, TRUE)) 
					std::cerr << "Unable to register console handler" << std::endl;
			}
		}
		
		void finalize()
		{
		}
	};
};

#endif
