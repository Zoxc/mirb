#include "../common.hpp"
#include "../char-array.hpp"

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
		
		bool is_file(const CharArray &path);
		bool is_directory(const CharArray &path);
		bool file_exists(const CharArray &path);
		CharArray cwd();
		void *allocate_region(size_t bytes);
		void free_region(void *region, size_t bytes);

		void initialize();
	};
};

#ifdef WIN32
	#include "winapi.hpp"
#else
	#include "posix.hpp"
#endif
