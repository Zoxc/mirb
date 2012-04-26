#pragma once
#include "allocator.hpp"
#include <Prelude/Vector.hpp>
#include "value.hpp"

namespace Mirb
{
	class Context
	{
		public:
			struct Symbols
			{
				Symbol *classpath;
				Symbol *classname;
				Symbol *attached;

				Symbol *terminator;
			};
			
			Class *object_class;
			Class *class_class;
			Class *array_class;
			Class *string_class;
			Class *proc_class;
			Class *symbol_class;
			Class *nil_class;
			Class *true_class;
			Class *false_class;
			Class *module_class;
			Class *exception_class;
			Class *method_class;
			Class *standard_error;
			Class *name_error;
			Class *type_error;
			Class *argument_error;
			Class *runtime_error;
			Class *local_jump_error;
			Class *fixnum_class;

			Module *kernel_module;

			Symbols syms;

			Class *class_of_table[literal_count];

			Object *main;

			Class *terminator;


			Context();
			
			template<typename F> void mark_fields(F mark)
			{
				auto start = &object_class;

				while(start != &terminator)
				{
					if(*start)
						mark(*start);

					start++;
				}
			}

			template<typename F> void mark(F mark)
			{
				mark_fields(mark);
			}
	};

	extern Context *context;
};
