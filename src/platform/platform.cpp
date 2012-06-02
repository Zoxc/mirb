#include "platform.hpp"
#include "../runtime.hpp"
#include "../classes/file.hpp"
#include "../classes/string.hpp"

namespace Mirb
{
	namespace Platform
	{
		CharArray join(const CharArray &left, const CharArray &right)
		{
			return File::join(left, right);
		}

		CharArray native_path(const CharArray &path)
		{
			if(!path.size())
				throw empty_path_error();

			return File::expand_path(path, cwd());
		}

		CharArray ruby_path(const CharArray &path)
		{
			return path;
		}
	};
};
