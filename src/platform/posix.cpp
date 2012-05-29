#include "platform.hpp"

#ifndef WIN32

#include "../collector.hpp"
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>

namespace Mirb
{
	namespace Platform
	{
		CharArray native_null_path(const CharArray &path)
		{
			return native_path(path).c_str();
		}
		
		CharArray ruby_path_from_null(const char *path)
		{
			return ruby_path(CharArray((const char_t *)path, std::strlen(path)));
		}

		void raise(const CharArray &message)
		{
			int num = errno;

			CharArray error = (const char_t *)strerror(num);

			CharArray msg = message + "\nError #" + CharArray::uint(num) + ": " + error;

			raise(context->system_call_error, msg);
		}
		
		double get_time()
		{
			timeval time;

			gettimeofday(&time, 0);

			return time.tv_sec + (double)time.tv_usec / 1000000;
		}

		size_t stack_start()
		{
			char dummy;

			size_t result = (size_t)&dummy;

			return result;
		}

		size_t stack_limit()
		{
			struct rlimit rlim;

			if(getrlimit(RLIMIT_STACK, &rlim) == 0)
				return std::min(rlim.rlim_cur, (rlim_t)(1024 * 1024 * 128));
			else
				return 1024 * 64;
		}
		
		std::string BenchmarkResult::format()
		{
			std::stringstream result;

			result << ((double)time / 1000) << " ms";

			return result.str();
		}

		void *allocate_region(size_t bytes)
		{
			void *result = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

			mirb_runtime_assert(result != 0);

			return result;
		}
		
		void free_region(void *region, size_t bytes)
		{
			munmap(region, bytes);
		}
		
		CharArray cwd()
		{
			char *result = getcwd(nullptr, 0);

			if(!result)
				raise("Unable to get the current directory");

			CharArray str = ruby_path_from_null(result);

			std::free(result);

			return str;
		}
		
		void mkdir(const CharArray &path)
		{
			auto dir = native_null_path(path);

			if(::mkdir(dir.c_str_ref(), 0777) != 0)
				raise("Unable to create directory '" + path + "'");
		}

		bool is_executable(const CharArray &path) throw()
		{
			try
			{
				auto file = native_null_path(path);

				return eaccess(file.c_str_ref(), X_OK) == 0;
			}
			catch(empty_path_error)
			{
				return false;
			}
		}

		bool stat(struct stat &buf, const CharArray &file)
		{
			try
			{
				auto path = native_null_path(file);

				return ::stat(path.c_str_ref(), &buf) == 0;
			}
			catch(empty_path_error)
			{
				return false;
			}
		}

		bool file_exists(const CharArray &file) throw()
		{
			struct stat buf;

			return stat(buf, file);
		}

		bool is_directory(const CharArray &path) throw()
		{
			struct stat buf;

			if(!stat(buf, path))
				return false;

			return S_ISDIR(buf.st_mode);
		}

		bool is_file(const CharArray &path) throw()
		{
			struct stat buf;

			if(!stat(buf, path))
				return false;

			return S_ISREG(buf.st_mode);
		}

		void signal_handler(int)
		{
			Collector::signal();
		}

		void initialize(bool console)
		{
			if(console)
				signal(SIGINT, signal_handler);
		}
		
		void finalize()
		{
		}
	};
};

#endif
