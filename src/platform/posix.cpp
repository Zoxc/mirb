#include "platform.hpp"

#ifndef WIN32

#include "../collector.hpp"
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>

namespace Mirb
{
	namespace Platform
	{
		void raise(const CharArray &message)
		{
			int num = errno;

			CharArray error = (const char_t *)strerror(num);

			CharArray msg = message + "\nError #" + CharArray::uint(num) + ": " + error;

			throw create_exception(context->system_call_error, msg);
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

			CharArray str((const char_t *)result, std::strlen(result));

			std::free(result);

			return str;
		}
		
		void cd(const CharArray &path)
		{
			CharArray path_cstr = path.c_str();

			if(chdir(path_cstr.c_str_ref()) == -1)
				raise("Unable change the current directory to '" + path + "'");
		}

		bool stat(struct stat &buf, const CharArray &file)
		{
			CharArray file_cstr = file.c_str();

			return ::stat(file_cstr.c_str_ref(), &buf) == 0;
		}

		bool file_exists(const CharArray &file)
		{
			struct stat buf;

			return stat(buf, file);
		}

		bool is_directory(const CharArray &path)
		{
			struct stat buf;

			if(!stat(buf, path))
				return false;

			return S_ISDIR(buf.st_mode);
		}

		bool is_file(const CharArray &path)
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
