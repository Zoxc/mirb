#pragma once
#include <sys/time.h>
#include <dirent.h>
#include <fcntl.h>

namespace Mirb
{
	namespace Platform
	{
		void raise(const CharArray &message);

		CharArray native_null_path(const CharArray &path);
		CharArray ruby_path_from_null(const char *path);

		class NativeStream:
			public Stream
		{
			protected:
				int fd;

			public:
				virtual void print(const CharArray &string);
				virtual pos_t pos();
				virtual pos_t size();
				virtual CharArray read(size_t length);
				virtual void seek(pos_t val, PosType type);

				NativeStream(int fd);
				~NativeStream();
		};

		class ConsoleStream:
			public NativeStream
		{
			public:
				virtual void color(Color color, const CharArray &string);

				ConsoleStream(int fd) : NativeStream(fd) {}
		};

		class BenchmarkResult
		{
			public:
				uint64_t data;

				double time();

				BenchmarkResult()
				{
					data = 0;
				}
		};

		template<typename T> void benchmark(BenchmarkResult &result, T work)
		{
			timeval start, stop;

			gettimeofday(&start, 0); 

			work();

			gettimeofday(&stop, 0); 

			result.data += ((uint64_t)stop.tv_sec * 1000000 + stop.tv_usec) - ((uint64_t)start.tv_sec * 1000000 + start.tv_usec);
		};

		template<typename F> void list_dir(const CharArray &path, bool skip_special, F func)
		{
			DIR *d;
			dirent *dir;

			auto path_cstr = native_null_path(path);

			d = opendir(path_cstr.c_str_ref());

			if(!d)
				raise("Unable to list directory: '" + path + "'");

			int fd = dirfd(d);

			if(fd == -1)
				raise("Unable to get directory descriptor");

			while((dir = readdir(d)) != 0)
			{

				if(skip_special && (strcmp(dir->d_name, "..") == 0 || strcmp(dir->d_name, ".") == 0))
					continue;

				struct stat st;

				CharArray name = (const char_t *)dir->d_name;

				if(fstatat(fd, dir->d_name, &st, AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW) != 0)
					raise("Unable to stat file: '" + join(path, name) + "'");

				func(name, S_ISDIR(st.st_mode));
			}

			closedir(d);
		}
	};
};
