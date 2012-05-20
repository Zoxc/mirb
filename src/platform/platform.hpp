#include "../common.hpp"
#include "../char-array.hpp"
#include "../classes/file.hpp"

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

		class Exception
		{
			private:
				CharArray message;

			public:
				Exception(const CharArray &message) : message(message) {}

				value_t raise() const;
		};
		
		template<typename F> value_t wrap(F func) throw()
		{
			try
			{
				return func();
			}
			catch(const Exception &exception)
			{
				return exception.raise();
			}
		}
		
		void cd(const CharArray &path);
		bool is_file(const CharArray &path);
		bool is_directory(const CharArray &path);
		bool file_exists(const CharArray &path);
		CharArray cwd();
		void *allocate_region(size_t bytes);
		void free_region(void *region, size_t bytes);
		
		CharArray expand_path(CharArray relative);
	
		void initialize();
	};
};

#ifdef WIN32
	#include "winapi.hpp"
#else
	#include "posix.hpp"
#endif
