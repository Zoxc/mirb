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

		PVOID exception_handler;

		void __fastcall stack_overflow_handler(_EXCEPTION_POINTERS *info)
		{
			// Restore guard page as described on http://support.microsoft.com/kb/315937
			
			LPBYTE stack; 
			SYSTEM_INFO system_info;
			MEMORY_BASIC_INFORMATION mem_info;
			DWORD old_protect;

			// Get page size of system

			GetSystemInfo(&system_info);
				
			#ifdef _AMD64_
				stack = (LPBYTE)info->ContextRecord->Rsp;
			#else
				stack = (LPBYTE)info->ContextRecord->Esp;
			#endif

			// Get allocation base of stack

			mirb_runtime_assert(VirtualQuery(stack, &mem_info, sizeof(mem_info)) != 0);

			// Go to page beyond current page

			stack = (LPBYTE)(mem_info.BaseAddress) - system_info.dwPageSize;

			// Free portion of stack just abandoned

			mirb_runtime_assert(VirtualFree(mem_info.AllocationBase, stack - (LPBYTE)mem_info.AllocationBase, MEM_DECOMMIT) != 0);

			// Reintroduce the guard page

			mirb_runtime_assert(VirtualProtect(stack, system_info.dwPageSize, PAGE_GUARD | PAGE_READWRITE, &old_protect) != 0);

			auto exception = Collector::allocate<Exception>(context->system_stack_error, String::get("Stack overflow"), backtrace(context->frame->prev));

			throw exception;
		}

		const size_t overflow_stack_pages = 10;
		
		LONG CALLBACK vectored_handler(_EXCEPTION_POINTERS *info)
		{
			if(info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW && context && context->frame)
			{
				static DWORD old_protect;

				auto new_stack = VirtualAlloc(0, 0x1000 * (overflow_stack_pages + 2), MEM_COMMIT, PAGE_READWRITE);

				mirb_runtime_assert(new_stack != 0);
				
				mirb_runtime_assert(VirtualProtect(new_stack, 0x1000, PAGE_GUARD | PAGE_READWRITE, &old_protect) != 0);

				new_stack = (LPVOID)((char *)new_stack + 0x1000 * (overflow_stack_pages + 1));

				mirb_runtime_assert(VirtualProtect(new_stack, 0x1000, PAGE_GUARD | PAGE_READWRITE, &old_protect) != 0);

#ifdef _MSC_VER
				__asm
				{
					mov ecx, info
					mov esp, new_stack
					call stack_overflow_handler
				};
#else
				asm("mov %0, %%rcx\nmov %1, %%rsp\ncall *%2" :: "g" (info), "g" (new_stack), "r" (&stack_overflow_handler));
#endif
			}

			return EXCEPTION_CONTINUE_SEARCH;
		}

		void initialize(bool console)
		{
			if(console)
			{
				SetConsoleTitle(TEXT("mirb"));

				if(!SetConsoleCtrlHandler(ctrl_handler, TRUE)) 
					std::cerr << "Unable to register console handler" << std::endl;
			}

			exception_handler = AddVectoredExceptionHandler(0, vectored_handler);

			if(!exception_handler)
				std::cerr << "Unable to register vectored exception handler" << std::endl;
		}
		
		void finalize()
		{
			if(exception_handler)
				RemoveVectoredExceptionHandler(exception_handler);
		}
	};
};

#endif