#include "../common.hpp"
#include "../char-array.hpp"
#include "../classes/file.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	namespace Platform
	{
		enum Color
		{
			Green,
			Bold,
			Red,
			Purple,
			Blue,
			Gray
		};

		size_t stack_start();
		size_t stack_limit();

		double get_time();

		struct empty_path_error {};

		template<typename F> void wrap(F func)
		{
			try
			{
				func();
			}
			catch(empty_path_error)
			{
				raise(context->system_call_error, "Empty path");
			}
		}
		
		CharArray native_path(const CharArray &path);
		CharArray ruby_path(const CharArray &path);
		
		bool is_file(const CharArray &path) throw();
		bool is_directory(const CharArray &path) throw();
		bool is_executable(const CharArray &path) throw();
		bool file_exists(const CharArray &path) throw();
		
		void mkdir(const CharArray &path);
		CharArray cwd();

		void *allocate_region(size_t bytes);
		void free_region(void *region, size_t bytes);
		
		void initialize(bool console);
		void finalize();
	};
};

#ifdef WIN32
	#include "winapi.hpp"
#else
	#include "posix.hpp"
#endif
