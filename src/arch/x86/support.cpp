#include "support.hpp"
#include "../../support.hpp"
#include "../../../runtime/classes/proc.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Support
		{
			rt_value __cdecl create_closure(Block *block, rt_value self, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[])
			{
				return Mirb::Support::create_closure(block, self, method_name, method_module, argc, argv);
			}

			rt_value __fastcall call(Symbol *method_name, rt_value dummy, rt_value obj, rt_value block, size_t argc, rt_value argv[])
			{
				return Mirb::Support::call(method_name, obj, block, argc, argv);
			}

			rt_value __fastcall super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
			{
				return Mirb::Support::super(method_name, method_module, obj, block, argc, argv);
			}
		};
	};
};

