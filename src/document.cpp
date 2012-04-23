#include "document.hpp"

namespace Mirb
{
	void Document::copy(const char_t *data, size_t length)
	{
		this->data = (const char_t *)std::malloc(length + 1);

		mirb_runtime_assert(this->data);

		std::memcpy((void *)this->data, data, length + 1);

		this->length = length;
	}
};
