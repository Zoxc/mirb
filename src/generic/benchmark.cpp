#include "benchmark.hpp"

namespace Mirb
{
	std::string BenchmarkResult::format()
	{
		std::stringstream result;

		#ifdef WIN32
			result << (((double)1000 * time.QuadPart) / (double)freq.QuadPart) << " ms";
		#else
			result << ((double)time / 1000) << " ms";
		#endif

		return result.str();
	}
};
