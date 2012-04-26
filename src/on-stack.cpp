#include "on-stack.hpp"
#include "char-array.hpp"

namespace Mirb
{
	OnStackBlock<false> *OnStackBlock<false>::current = nullptr;
	OnStackBlock<true> *OnStackBlock<true>::current = nullptr;
	
	value_t **OnStackBase<false>::get_refs()
	{
		return &static_cast<OnStack<1> *>(this)->refs[0];
	}
	
	CharArray **OnStackBase<true>::get_refs()
	{
		return &static_cast<OnStackString<1> *>(this)->refs[0];
	}
	
	void OnStackBase<false>::push(value_t &arg, size_t &index)
	{
		get_refs()[index++] = &arg;
	}
	
	void OnStackBase<true>::push(CharArray &arg, size_t &index)
	{
		get_refs()[index++] = &arg;
	}

	void OnStackBase<true>::push(const CharArray &arg, size_t &index)
	{
		get_refs()[index++] = const_cast<CharArray *>(&arg);
	}
};
