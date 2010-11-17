#pragma once
#include "../../common.hpp"
#include "../../block.hpp"
#include "../../../runtime/classes.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Support
		{
			rt_value __cdecl create_closure(Block *block, rt_value self, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[]);

			rt_value __fastcall call(Symbol *method_name, rt_value dummy, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
			rt_value __fastcall super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
		};
	};
};
