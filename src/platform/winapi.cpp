#include "platform.hpp"

#ifdef WIN32

#include "../collector.hpp"

namespace Mirb
{
	namespace Platform
	{
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

				return CharArray((const char_t *)new_buffer, size);
			#else
				return CharArray((const char_t *)buffer, tchars);
			#endif
		}

		CharArray cwd()
		{
			size_t size = GetCurrentDirectory(0, 0);

			TCHAR *buffer = (TCHAR *)alloca(size * sizeof(TCHAR));

			size = GetCurrentDirectory(size, buffer);

			return from_tchar(buffer, size);
		}
		
		BOOL __stdcall ctrl_handler(DWORD ctrl_type) 
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
 
		void initialize()
		{
			SetConsoleTitle(TEXT("mirb"));

			if(!SetConsoleCtrlHandler(ctrl_handler, TRUE)) 
				std::cerr << "Unable to register console handler" << std::endl; 
		}
	};
};

#endif