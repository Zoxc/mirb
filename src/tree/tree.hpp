#pragma once
#include "../../runtime/runtime.hpp"
#include "../common.hpp"
#include "../simpler-list.hpp"
#include "../vector.hpp"
#include "nodes.hpp"

namespace Mirb
{
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
				
				Type type;
				Scope *owner; // Only valid for heap variables
				
				Variable(Type type) : type(type) {}

				#ifdef DEBUG
					SimplerEntry<Variable> entry;
				#endif

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
			public HashTableFunctions<Symbol *, NamedVariable *, Fragment *>
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
				
				static NamedVariable **alloc(Fragment *fragment, size_t entries);
				
				static void free(Fragment *fragment, NamedVariable **table, size_t entries)
				{
				}
		};
		
		typedef HashTable<Symbol *, NamedVariable *, Fragment *, VariableMapFunctions> VariableMap;
		
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
				Fragment *fragment;
				Type type;
				Scope *parent; // The scope enclosing this one
				Scope *owner; // The first parent that isn't a closure. This field can point to itself.
				Node *group;
				
				/*
				 * Break related fields
				 */
				bool can_break; // If this block can raise a exception because of a break.
				size_t break_id; // Which of the parent's break targets this block belongs to.
				size_t break_targets; // Number of child blocks that can raise a break exception.
				
				/*
				 * Exception related fields
				 */
				bool require_exceptions;

				/*
				 * Variable related fields
				 */
				VariableMap variables;
				size_t local_vars; // The number of the local and temporary variables that is not stored on the heap.
				size_t heap_vars; // The number of the variables that must be stored on a heap scope.
				Parameter *block_parameter; // Pointer to a named or unnamed block variable.
				NamedVariable *super_module_var; // Pointer to a next module to search from if super is used.
				NamedVariable *super_name_var; // Pointer to a symbol which contains the name of the called method.
				Vector<Scope *, Fragment> referenced_scopes; // A list of all the scopes this scope requires.
				
				#ifdef DEBUG
					SimplerList<Variable> variable_list;
				#endif

				SimplerList<Parameter, Parameter, &Parameter::parameter_entry> parameters;
				
				Vector<Scope *, Fragment> zsupers;
				
				template<class T> T *alloc_var(Variable::Type type = Variable::Local)
				{
					T *result = new (fragment) T(type);
					
					result->index = local_vars++;
					
					result->type = type;

					variable_list.append(result);
					
					return result;
				}
				
				template<class T> T *define(Symbol *name)
				{
					T *result = (T *)variables.get(name);
					
					if(result)
						return result;
					
					result = alloc_var<T>();
					
					result->name = name;

					#ifdef DEBUG
						variables.set(name, result);
					#endif

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
				
				Scope *defined(Symbol *name, bool recursive);
		};
	};
};

void *operator new(size_t bytes, Mirb::Tree::Fragment *fragment) throw();
void operator delete(void *, Mirb::Tree::Fragment *fragment) throw();
void *operator new[](size_t bytes, Mirb::Tree::Fragment *fragment) throw();
void operator delete[](void *, Mirb::Tree::Fragment *fragment) throw();
