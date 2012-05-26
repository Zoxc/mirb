#include "platform.hpp"

#ifdef WIN32

#include "../classes/string.hpp"
#include "../collector.hpp"
#include "../runtime.hpp"

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
			
			throw create_exception(context->system_call_error, msg);
		}
		
		size_t stack_start()
		{
			#ifdef _AMD64_
				return (size_t)__readfsqword(offsetof(NT_TIB, StackBase));
			#else
				return (size_t)__readfsdword(offsetof(NT_TIB, StackBase));
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
				size_t size = WideCharToMultiByte(CP_UTF8, 0, buffer, tchars, nullptr, 0, NULL, NULL);

				char *new_buffer = (char *)alloca(size);

				size = WideCharToMultiByte(CP_UTF8, 0, buffer, tchars, new_buffer, size, NULL, NULL);

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

				return normalize_separator_from(result);
			#else
				return normalize_separator_from(path);
			#endif
		}
	
		CharArray to_win_path(const CharArray &path)
		{
			#ifdef _UNICODE
				return "\\\\?\\" + normalize_separator_to(path);
			#else
				return normalize_separator_to(path);
			#endif
		}
	
		CharArray from_short_win_path(const TCHAR *buffer)
		{
			#ifdef _UNICODE
				size_t size = GetLongPathName(buffer, 0, 0);

				TCHAR *long_buffer = (TCHAR *)alloca((size + 1) * sizeof(TCHAR));

				if(GetLongPathName(buffer, long_buffer, size + 1) == 0)
					raise("Unable to convert short pathname to a long pathname");

				return from_win_path(from_tchar(long_buffer));
			#else
				return from_win_path(from_tchar(buffer));
			#endif
		}
	
		DWORD attributes(const CharArray &path)
		{
			DWORD attrib; 

			to_tchar(to_win_path(path), [&](const TCHAR *buffer, size_t) {
				attrib = GetFileAttributes(buffer);
			});

			return attrib;
		}
		
		CharArray cwd()
		{
			size_t size = GetCurrentDirectory(0, 0);

			TCHAR *buffer = (TCHAR *)alloca(size * sizeof(TCHAR));

			if(GetCurrentDirectory(size, buffer) == 0)
				raise("Unable to get the current directory");

			return from_short_win_path(buffer);
		}
		
		void cd(const CharArray &path)
		{
			to_short_win_path(path, [&](const TCHAR *buffer) {
				if(SetCurrentDirectory(buffer) == 0)
					raise("Unable change the current directory to '" + path + "'");
			});
		}

		bool file_exists(const CharArray &path)
		{
			return (attributes(path) != INVALID_FILE_ATTRIBUTES);
		}
		
		bool is_directory(const CharArray &path)
		{
			DWORD attrib = attributes(path); 

			return ((attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		bool is_file(const CharArray &path)
		{
			DWORD attrib = attributes(path); 

			return ((attrib != INVALID_FILE_ATTRIBUTES) && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
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