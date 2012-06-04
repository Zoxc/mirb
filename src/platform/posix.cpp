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
		
		double BenchmarkResult::time()
		{
			return (double)data / 1000;
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
		
		void remove_dir(const CharArray &path)
		{
			auto dir = native_null_path(path);

			if(::rmdir(dir.c_str_ref()) != 0)
				raise("Unable to remove directory '" + path + "'");
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

		NativeStream *open(const CharArray &path, size_t access, Mode mode)
		{
			auto file = native_null_path(path);

			int flags = 0;

			if(access & Read && access & Write)
				flags = O_RDWR;
			else if(access & Read)
				flags = O_RDONLY;

			else if(access & Write)
				flags = O_WRONLY;
			else
				mirb_debug_assert("No access specified");

			if(mode != Open)
				flags |= O_CREAT;

			int fd = ::open(file.c_str_ref(), flags, 0777);

			if(fd == -1)
				raise("Unable to open file '" + path + "'");

			switch(mode)
			{
				case CreateTruncate:
					if(ftruncate(fd, 0) == -1)
					{
						close(fd);
						raise("Unable to truncate file '" + path + "'");
					}
					break;

				case CreateAppend:
					if(lseek(fd, 0, SEEK_END) == -1)
					{
						close(fd);
						raise("Unable to seek to end of file '" + path + "'");
					}
					break;

				default:
					break;
			}

			return new NativeStream(fd);
		}

		NativeStream::NativeStream(int fd) : fd(fd)
		{
		}

		NativeStream::~NativeStream()
		{
			close(fd);
		}

		void NativeStream::print(const CharArray &string)
		{
			if(write(fd, string.str_ref(), string.size()) == -1)
				raise("Unable to write to file descriptor");
		}

		CharArray NativeStream::read(size_t length)
		{
			CharArray buf;
			buf.buffer(length);

			ssize_t result = ::read(fd, buf.str_ref(), buf.size());

			if(result < 0)
				raise("Unable to read from file descriptor");

			buf.shrink((size_t)result);
			return buf;
		}

		Stream::pos_t NativeStream::pos()
		{
			off_t result = lseek(fd, 0, SEEK_CUR);

			if(result == (off_t)-1)
				raise("Unable to get stream position");

			return result;
		}

		Stream::pos_t NativeStream::size()
		{
			struct stat st;

			if(fstat(fd, &st) != 0)
				raise("Unable to get file size from descriptor");

			return st.st_size;
		}

		void NativeStream::seek(pos_t val, PosType type)
		{
			int pos;

			switch(type)
			{
				case FromStart:
					pos = SEEK_SET;
					break;

				case FromCurrent:
					pos = SEEK_CUR;
					break;

				case FromEnd:
					pos = SEEK_END;
					break;
			};

			if(lseek(fd, val, pos) == (off_t)-1)
				raise("Unable to seek in file descriptor");
		}

		ConsoleStream *console_stream(ConsoleStreamType type)
		{
			int fd;

			switch(type)
			{
				case StandardInput:
					fd = fcntl(STDIN_FILENO, F_DUPFD, 0);
					break;

				case StandardOutput:
					fd = fcntl(STDOUT_FILENO, F_DUPFD, 0);
					break;

				case StandardError:
					fd = fcntl(STDERR_FILENO, F_DUPFD, 0);
					break;
			}

			if(fd == -1)
				raise("Unable to get console stream");

			return new ConsoleStream(fd);
		}

		void ConsoleStream::color(Color color, const CharArray &string)
		{
			switch(color)
			{
				case Red:
					print("\033[1;31m" + string + "\033[0m");
					break;

				case Blue:
					print("\033[1;34m" + string + "\033[0m");
					break;

				case Green:
					print("\033[1;32m" + string + "\033[0m");
					break;

				case Bold:
					print("\033[1m" + string + "\033[0m");
					break;

				default:
					print(string);
					return;
			};
		}

		void signal_handler(int)
		{
			Collector::signal();
		}

		void initialize()
		{
			if(context->console)
				signal(SIGINT, signal_handler);
		}
		
		void finalize()
		{
		}
	};
};

#endif
