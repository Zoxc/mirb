#include "platform.hpp"
#include "../runtime.hpp"
#include "../classes/file.hpp"
#include "../classes/string.hpp"

namespace Mirb
{
	namespace Platform
	{
		CharArray native_path(const CharArray &path)
		{
			if(!path.size())
				raise(context->system_call_error, "Empty path");

			return File::expand_path(path, cwd());
		}

		CharArray ruby_path(const CharArray &path)
		{
			return path;
		}

		void ColorWrapperBase::print(const CharArray &string)
		{
			auto out_str = try_cast<String>(output);

			if(out_str)
				out_str->string += string;
			else
				std::cerr << string.get_string();
		}
	};
};
