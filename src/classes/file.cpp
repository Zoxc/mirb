#include "file.hpp"
#include "../runtime.hpp"
#include "string.hpp"

namespace Mirb
{
	CharArray normalize_path(const CharArray &path)
	{
		CharArray result(path);

		result.localize();

		for(size_t i = 0; i < result.size(); ++i)
			if(result[i] == '\\')
				result[i] = '/';

		if(result[result.size()-1] == '/')
			result.shrink(result.size() - 1);

		return result;
	}

	void JoinSegments::push(const CharArray &path)
	{
		normalize_path(path).split([&](const CharArray &part) {
			segments.push_back(part);
		}, CharArray("/"));
	}

	CharArray JoinSegments::join()
	{
		int pos = 0;

		CharArray result;

		for(size_t i = segments.size(); i-- > 0;)
		{
			if(segments[i] == ".")
				continue;
				
			if(segments[i] == "..")
			{
				pos--;
				continue;
			}

			if(pos < 0)
				pos++;
			else
				result = segments[i] + "/" + result;
		}

		if(result.size())
			result.shrink(result.size() - 1);

		return result;
	}

	value_t expand_path(value_t file_name, value_t path)
	{
		auto filename = cast<String>(file_name);
		auto filepath = cast<String>(path);
		
		JoinSegments joiner;
		
		joiner.push(filepath->string);
		joiner.push(filename->string);
		
		return joiner.join().to_string();
	}

	void File::initialize()
	{
		context->file_class = define_class("File", context->io_class);

		singleton_method<Arg::Value, Arg::Value>(context->file_class, "expand_path", &expand_path);
	}
};

