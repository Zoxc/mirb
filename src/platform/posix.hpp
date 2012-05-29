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

		template<Color input> void color(const CharArray &string)
		{
			switch(input)
			{
				case Red:
					std::cerr << ("\033[1;31m" + string + "\033[0m").get_string();
					break;

				case Blue:
					std::cerr << ("\033[1;34m" + string + "\033[0m").get_string();
					break;

				case Green:
					std::cerr << ("\033[1;32m" + string + "\033[0m").get_string();
					break;

				case Bold:
					std::cerr << ("\033[1m" + string + "\033[0m").get_string();
					break;

				default:
					std::cerr << string.get_string();
					return;
			};
		};

		class BenchmarkResult
		{
			public:
				uint64_t time; 

				std::string format();
		};

		template<typename T> BenchmarkResult benchmark(T work)
		{
			BenchmarkResult result;

			timeval start, stop;

			gettimeofday(&start, 0); 

			work();

			gettimeofday(&stop, 0); 

			result.time = ((uint64_t)stop.tv_sec * 1000000 + stop.tv_usec) - ((uint64_t)start.tv_sec * 1000000 + start.tv_usec);
			
			return result;
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
					raise("Unable to stat file: '" + File::join(path, name) + "'");

				func(name, S_ISDIR(st.st_mode));
			}

			closedir(d);
		}
	};
};
