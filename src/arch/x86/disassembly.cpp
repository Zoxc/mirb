extern "C"
{
	#include <udis86.h>
};

#include "../../mem_stream.hpp"
#include "disassembly.hpp"
#include "support.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Disassembly
		{
			#define MIRB_SYMBOL(name) {(void *)&name, #name}

			static Symbol symbols[] = {
				/*MIRB_SYMBOL(rt_support_closure),
				MIRB_SYMBOL(rt_support_define_class),
				MIRB_SYMBOL(rt_support_define_module),
				MIRB_SYMBOL(rt_support_define_method),*/
				MIRB_SYMBOL(rt_support_call),
				MIRB_SYMBOL(rt_support_super),
				/*MIRB_SYMBOL(rt_support_get_ivar),
				MIRB_SYMBOL(rt_support_set_ivar),
				MIRB_SYMBOL(rt_support_break),
				MIRB_SYMBOL(rt_support_return),
				MIRB_SYMBOL(rt_support_alloc_scope),

				#ifdef WIN32
					MIRB_SYMBOL(rt_support_handler),
				#endif

				MIRB_SYMBOL(rt_support_interpolate),
				MIRB_SYMBOL(rt_support_array),
				MIRB_SYMBOL(rt_support_get_const),
				MIRB_SYMBOL(rt_support_set_const),
				MIRB_SYMBOL(rt_support_define_string)*/
			};

			const char *find_symbol(void *address, Vector<Symbol *, MemoryPool> *symbols)
			{
				for(size_t i = 0; i < sizeof(Disassembly::symbols) / sizeof(Symbol); i++)
				{
					if(Disassembly::symbols[i].address == address)
						return Disassembly::symbols[i].symbol;
				}

				if(symbols)
				{
					for(auto i = symbols->begin(); i != symbols->end(); ++i)
					{
						if(i()->address == address)
							return i()->symbol;
					}
				}

				return 0;
			}

			void dump_hex(unsigned char* address, int length)
			{
				int min_length = 7 - length;

				int index = 0;

				while(index < length)
					printf(" %.2X", (size_t)*(address + index++));

				while(min_length-- > 0)
					printf("   ");
			}

			void dump_instruction(ud_t* ud_obj, Vector<Symbol *, MemoryPool> *symbols)
			{
				printf("0x%08X:", (size_t)ud_insn_off(ud_obj));

				dump_hex(ud_insn_ptr(ud_obj), ud_insn_len(ud_obj));

				printf(" %s", ud_insn_asm(ud_obj));
	
				const char *symbol = 0;
	
				for(size_t i = 0; !symbol && i < 3; i++)
				{
					switch(ud_obj->operand[i].type)
					{
						case UD_OP_IMM:
						case UD_OP_MEM:
							symbol = find_symbol((void *)ud_obj->operand[i].lval.udword, symbols);
							break;
			
						case UD_OP_PTR:
							symbol = find_symbol((void *)ud_obj->operand[i].lval.ptr.off, symbols);
							break;
			
						case UD_OP_JIMM:
							symbol = find_symbol((void *)((size_t)ud_insn_off(ud_obj) + ud_obj->operand[i].lval.udword + ud_insn_len(ud_obj)), symbols);
							break;
			
						default:
							break;
					}
				}
	
				if(symbol)
					printf(" ;%s", symbol);
	
				printf("\n");
			}

			void dump_code(MemStream &stream, Vector<Symbol *, MemoryPool> *symbols)
			{
				ud_t ud_obj;

				ud_init(&ud_obj);
				ud_set_input_buffer(&ud_obj, (uint8_t *)stream.pointer(), stream.size());
				ud_set_mode(&ud_obj, 32);
				ud_set_syntax(&ud_obj, UD_SYN_INTEL);
				ud_set_pc(&ud_obj, (size_t)stream.pointer());

				while(ud_disassemble(&ud_obj))
				{
					dump_instruction(&ud_obj, symbols);
				}
			}
		};
	};
};
