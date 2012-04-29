#include "platform.hpp"
#include "../collector.hpp"

#ifdef WIN32

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
			SetConsoleTitle("mirb");

			if(!SetConsoleCtrlHandler(ctrl_handler, TRUE)) 
				std::cerr << "Unable to register console handler" << std::endl; 
		}
	};
};

#endif