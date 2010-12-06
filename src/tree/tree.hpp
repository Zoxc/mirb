#pragma once
#include "../common.hpp"
#include "../generic/simpler-list.hpp"
#include "../generic/vector.hpp"
#include "../generic/simple-set.hpp"
#include "nodes.hpp"

namespace Mirb
{
	class Block;

	namespace CodeGen
	{
		class Block;
	};
	
	namespace Tree
	{
		class Chunk
		{
			public:
				static const size_t main_size = 512;
				static const size_t block_size = 256;
				static const size_t allocation_limit = 128;
				
				static Chunk *create(size_t bytes);
				
				uint8_t *current;
				uint8_t *end;
				
				SimplerEntry<Chunk> entry;
				
				void *allocate(size_t bytes);
		};
		
		class Fragment:
			public Allocator
		{
			public:
				typedef Fragment &Ref;
				typedef Allocator::Wrap<Fragment &> Storage;
				
				Fragment(Fragment *parent, size_t chunk_size);
				
				Vector<Fragment *, Fragment> fragments;
				
				void *alloc(size_t bytes);
				void *realloc(void *mem, size_t old_size, size_t new_size);
				void free(void *mem)
				{
				}
				
			private:
				size_t chunk_size;
				
				Chunk *current;
				
				SimplerList<Chunk> chunks;
		};
		
		struct Node;
		
		struct SuperNode;

		class LiveRange
		{
			public:
				size_t start;
				size_t stop;

				LiveRange() : start((size_t)-1), stop(0) {}

				bool contains(size_t loc)
				{
					return (start <= loc && stop >= loc);
				}

				void update_start(size_t loc)
				{
					start = std::min(start, loc);
				}

				void update_stop(size_t loc)
				{
					stop = std::max(stop, loc);
				}
		};
		
		class Variable
		{
			public:
				enum Type
				{
					Temporary,
					Local,
					Heap,
					Types
				};

				enum Flags
				{
					Register,
					FlushCallerSavedRegisters,
					FlushRegisters
				};
				
				Type type;
				Scope *owner; // Only valid for heap variables
				
				Variable(Type type) : type(type) {}

				LiveRange range;

				SimpleSet flags;

				size_t loc;

				size_t index;
		};

		class NamedVariable:
			public Variable
		{
			public:
				NamedVariable(Type type) : Variable(type) {}
				
				Symbol *name;
				NamedVariable *next;
		};

		class Parameter:
			public NamedVariable
		{
			public:
				Parameter(Type type) : NamedVariable(type) {}
				
				SimplerEntry<Parameter> parameter_entry;
		};

		class VariableMapFunctions:
			public HashTableFunctions<Symbol *, NamedVariable *, Fragment>
		{
			public:
				static bool compare_key_value(Symbol *key, NamedVariable *value)
				{
					return key == value->name;
				}

				static Symbol *get_key(NamedVariable *value)
				{
					return value->name;
				}

				static NamedVariable *get_value_next(NamedVariable *value)
				{
					return value->next;
				}

				static void set_value_next(NamedVariable *value, NamedVariable *next)
				{
					value->next = next;
				}
				
				static size_t hash_key(Symbol *key)
				{
					return (size_t)key;
				}
				
				static bool valid_key(Symbol *key)
				{
					return true;
				}
		};
		
		typedef HashTable<Symbol *, NamedVariable *, VariableMapFunctions, Fragment> VariableMap;
		
		class Scope
		{
			public:
				enum Type
				{
					Top,
					Method,
					Class,
					Module,
					Closure
				};
				
				Scope(Fragment &fragment, Scope *parent, Type type);
				
				CodeGen::Block *block;
				Block *final;
				Fragment *fragment;
				Type type;
				Scope *parent; // The scope enclosing this one
				Scope *owner; // The first parent that isn't a closure. This field can point to itself.
				Node *group;
				
				/*
				 * Break related fields
				 */
				size_t break_id; // Which of the parent's break targets this block belongs to.
				size_t break_targets; // Number of child blocks that can raise a break exception.
				
				static const size_t no_break_id = (size_t)-1;

				/*
				 * Exception related fields
				 */
				bool require_exceptions;

				/*
				 * Variable related fields
				 */
				VariableMap variables; // A hash of the variables in this scope.
				size_t heap_vars; // The number of the variables that must be stored on a heap scope.
				Parameter *block_parameter; // Pointer to a named or unnamed block variable.
				NamedVariable *module_var; // Pointer to a module in which this method was found.
				NamedVariable *name_var; // Pointer to a symbol which contains the name of the called method.
				Vector<Scope *, Fragment> referenced_scopes; // A list of all the scopes this scope requires.
				
				Vector<Variable *, Fragment> variable_list; // A list of all variables in this scope.

				SimplerList<Parameter, Parameter, &Parameter::parameter_entry> parameters;
				
				Vector<Scope *, Fragment> zsupers;
				
				template<class T> T *alloc_var(Variable::Type type = Variable::Local)
				{
					T *result = new (fragment) T(type);
					
					result->index = variable_list.size();
					
					result->type = type;

					variable_list.push(result);
					
					return result;
				}
				
				template<class T> T *define(Symbol *name)
				{
					T *result = (T *)variables.get(name);
					
					if(result)
						return result;
					
					result = alloc_var<T>();
					
					result->name = name;

					variables.set(name, result);

					return result;
				}
				
				template<class T> T *get_var(Symbol *name)
				{
					Scope *defined_scope = defined(name, true);

					if(defined_scope == this)
					{
						return variables.get(name);
					}
					else if(defined_scope)
					{
						auto result = defined_scope->variables.get(name);
						
						require_var(defined_scope, result);
						
						return result;
					}
					else
						return define<T>(name);
				}
				
				void require_args(Scope *owner);
				void require_scope(Scope *scope);
				void require_var(Scope *owner, Variable *var);

				void gen_ident_vars();
				
				Scope *defined(Symbol *name, bool recursive);
		};
	};
};

void *operator new(size_t bytes, Mirb::Tree::Fragment *fragment) throw();
void operator delete(void *, Mirb::Tree::Fragment *fragment) throw();
void *operator new[](size_t bytes, Mirb::Tree::Fragment *fragment) throw();
void operator delete[](void *, Mirb::Tree::Fragment *fragment) throw();
