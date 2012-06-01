#pragma once
#include "../common.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"
#include "../stream.hpp"

namespace Mirb
{
	namespace Platform
	{
		enum Access
		{
			Read = 1,
			Write = 2
		};
		
		enum Mode
		{
			Open,
			CreateTruncate,
			CreateAppend
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
		
		struct ColorWrapperBase
		{
			value_t output;
			OnStack<1> os;

			ColorWrapperBase(value_t output) : output(output), os(output) {}

			void print(const CharArray &string);
		};

		CharArray join(const CharArray &left, const CharArray &right);
		CharArray native_path(const CharArray &path);
		CharArray ruby_path(const CharArray &path);
		
		class NativeStream;

		NativeStream *open(const CharArray &path, size_t access, Mode mode);
		
		class ConsoleStream;

		enum ConsoleStreamType
		{
			StandardInput,
			StandardOutput,
			StandardError
		};

		ConsoleStream *console_stream(ConsoleStreamType type);

		bool is_file(const CharArray &path) throw();
		bool is_directory(const CharArray &path) throw();
		bool is_executable(const CharArray &path) throw();
		bool file_exists(const CharArray &path) throw();
		
		void remove_dir(const CharArray &path);
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

