#pragma once
#include "../common.hpp"
#include <Prelude/HashTable.hpp>
#include <Prelude/Vector.hpp>
#include <Prelude/FastList.hpp>
#include "../generic/simple-set.hpp"
#include "../collector.hpp"
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
				
				ListEntry<Chunk> entry;
				
				void *allocate(size_t bytes);
		};
		
		class FragmentBase
		{
			public:
				static const bool can_free = false;

				FragmentBase(size_t chunk_size);
				
				void *allocate(size_t bytes);
				void *reallocate(void *memory, size_t old_size, size_t new_size);
				void free(void *memory)
				{
				}
				
			private:
				size_t chunk_size;
				
				Chunk *current;
				
				FastList<Chunk> chunks;
		};

		typedef Prelude::Allocator::ReferenceTemplate<FragmentBase> Fragment;
		
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
				
				Variable(Type type)
					: type(type)
					#ifdef DEBUG
						, group(0)
					#endif
				{
				}

				size_t loc;

				#ifdef DEBUG
					Variable *group;
				#endif
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
				
				ListEntry<Parameter> parameter_entry;
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
		
		class Scope:
			public PinnedHeader
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
				
				Scope(Document *document, Fragment fragment, Scope *parent, Type type);
				
				Document *document;
				Block *final;
				FragmentBase *fragment;
				Type type;
				Scope *parent; // The scope enclosing this one
				Scope *owner; // The first parent that isn't a closure. This field can point to itself.
				Node *group;
				
				/*
				 * Break related fields
				 */
				size_t break_id; // Which of the parent's break targets this block belongs to.
				size_t break_targets; // Number of child blocks that can raise a break exception.
				var_t break_dst; // The variable in the parent that the break will override.
				
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
				Vector<Scope *, Fragment> referenced_scopes; // A list of all the scopes this scope requires.

				Vector<Scope *, Fragment> children; // A list of all the immediate children. To keep them alive...
				
				Vector<Variable *, Fragment> variable_list; // A list of all variables in this scope.

				CountedList<Parameter, Parameter, &Parameter::parameter_entry> parameters;
				
				Vector<Scope *, Fragment> zsupers;
				
				template<typename F> void mark(F mark)
				{
					if(final)
						mark(final);

					mark(document);
					
					referenced_scopes.mark_content(mark);
					children.mark_content(mark);
					zsupers.mark_content(mark);

					if(parent)
						mark(parent);
					
					mark(owner);
				}

				template<class T> T *alloc_var(Variable::Type type = Variable::Local)
				{
					T *result = new (fragment) T(type);
					
					result->loc = variable_list.size();
					
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

				Scope *defined(Symbol *name, bool recursive);
		};
	};
};

void *operator new(size_t bytes, Mirb::Tree::Fragment fragment) throw();
void operator delete(void *, Mirb::Tree::Fragment fragment) throw();
void *operator new[](size_t bytes, Mirb::Tree::Fragment fragment) throw();
void operator delete[](void *, Mirb::Tree::Fragment fragment) throw();
