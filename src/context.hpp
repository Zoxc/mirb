#pragma once
#include "allocator.hpp"
#include <Prelude/Vector.hpp>
#include "value-map.hpp"

namespace Mirb
{
	class Context
	{
		public:
			struct Symbols
			{
				Symbol *classpath;
				Symbol *class_scope;
				Symbol *classname;
				Symbol *attached;
				Symbol *compare;

				Symbol *terminator;
			};
			
			Class *object_class;
			Class *class_class;
			Class *array_class;
			Class *hash_class;
			Class *string_class;
			Class *regexp_class;
			Class *proc_class;
			Class *symbol_class;
			Class *nil_class;
			Class *true_class;
			Class *false_class;
			Class *module_class;
			Class *method_class;
			Class *dir_class;
			Class *range_class;
			
			Class *io_class;
			Class *file_class;
			
			Class *numeric_class;
			Class *fixnum_class;
			Class *float_class;

			Class *exception_class;

			Class *signal_exception;
			Class *interrupt_class;

			Class *standard_error;
			Class *name_error;
			Class *type_error;
			Class *syntax_error;
			Class *argument_error;
			Class *runtime_error;
			Class *local_jump_error;
			Class *load_error;

			Module *kernel_module;
			Module *comparable_module;
			Module *enumerable_module;
			Module *process_module;

			Method *inspect_method;

			Tuple<Module> *object_scope;

			Symbols syms;

			Class *class_of_table[literal_count];

			Object *main;
			
			Class *terminator;
			
			ValueMap::MapType globals;
			Vector<value_t, AllocatorBase, Allocator> at_exits;
			Vector<value_t, AllocatorBase, Allocator> loaded;

			Context();
			
			template<typename F> void mark(F mark)
			{
				auto start = &object_class;

				while(start != &terminator)
				{
					if(*start)
						mark(*start);

					start++;
				}
				
				at_exits.mark(mark);
				loaded.mark(mark);
				globals.mark(mark);
			}

			void setup();
	};

	extern Context *context;
};
