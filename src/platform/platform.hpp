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
		
		void cd(const CharArray &path);
		bool is_file(const CharArray &path);
		bool is_directory(const CharArray &path);
		bool file_exists(const CharArray &path);
		CharArray cwd();
		void *allocate_region(size_t bytes);
		void free_region(void *region, size_t bytes);
		
		CharArray expand_path(CharArray relative);
	
		void initialize(bool console);
		void finalize();
	};
};

#ifdef WIN32
	#include "winapi.hpp"
#else
	#include "posix.hpp"
#endif
