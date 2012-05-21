#pragma once
#include "../common.hpp"
#include <Prelude/HashTable.hpp>
#include <Prelude/Vector.hpp>
#include <Prelude/FastList.hpp>
#include "../collector.hpp"
#include "nodes.hpp"

namespace Mirb
{
	class Block;
	class Parser;

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
				static const bool null_references = false;

				FragmentBase(size_t chunk_size);
				
				~FragmentBase()
				{
					Chunk *c = chunks.first;
			
					while(c)
					{
						Chunk *next = c->entry.next;

						std::free(c);

						c = next;
					}
				}
				
				void *allocate(size_t bytes);
				void *reallocate(void *memory, size_t old_size, size_t new_size);
				void free(void *)
				{
				}
				
			private:
				const size_t chunk_size;
				
				Chunk *current;
				
				FastList<Chunk> chunks;
		};

		typedef Prelude::Allocator::ReferenceTemplate<FragmentBase> Fragment;
		
		struct Node;
		
		struct SuperNode;

		class Variable
		{
			public:
				bool heap : 1;
				bool has_default_value : 1;
				bool parameter_group : 1;

				Scope *owner; // Only valid for heap variables
				
				Variable() : heap(false), has_default_value(false), parameter_group(false)
				{
				}

				size_t loc;
		};

		class NamedVariable:
			public Variable
		{
			public:
				NamedVariable() : name(nullptr) {}
				
				Symbol *name;
				NamedVariable *next;
		};

		class Parameter:
			public NamedVariable
		{
			public:
				Parameter() : NamedVariable(), reported(false), node(nullptr) {}

				bool reported;
				Node *node;
				SourceLoc range;
				
				ListEntry<Parameter> parameter_entry;
		};

		class VariableMapFunctions:
			public HashTableFunctions<Symbol *, NamedVariable *, Fragment>
		{
			public:
				static bool compare_key_value(Symbol *key, size_t, NamedVariable *value)
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
				
				static bool valid_key(Symbol *)
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

				static const bool finalizer = true;

				~Scope()
				{
					if(type == Tree::Scope::Closure || type == Tree::Scope::Method || type == Tree::Scope::Top)
						delete fragment;
				}

				bool defered()
				{
					return type == Tree::Scope::Closure || type == Tree::Scope::Method;
				}

				void parse_done(Parser &parser);
				
				VoidTrapper trapper;
				List<VoidTrapper> trapper_list;
				Document *document;
				Block *final;
				FragmentBase *fragment;
				Type type;
				Scope *parent; // The scope enclosing this one
				Scope *owner; // The first parent that isn't a closure. This field can point to itself.
				Node *group;
				SourceLoc *range;
				
				/*
				 * Break related fields
				 */
				size_t break_id; // Which of the parent's break targets this block belongs to.
				size_t break_targets; // Number of child blocks that can raise a break exception.
				var_t break_dst; // The variable in the parent that the break will override.
				
				static const size_t no_break_id = (size_t)-1;

				/*
				 * Variable related fields
				 */
				VariableMap variables; // A hash of the variables in this scope.
				size_t heap_vars; // The number of the variables that must be stored on a heap scope.
				Vector<Scope *, Fragment> referenced_scopes; // A list of all the scopes this scope requires.

				Vector<Scope *, Fragment> children; // A list of all the immediate children. To keep them alive...
				
				Vector<Variable *, Fragment> variable_list; // A list of all variables in this scope.
				
				Parameter *block_parameter; // Pointer to a named or unnamed block variable.
				Parameter *array_parameter; // Pointer to a named or unnamed array variable.
				Vector<Parameter *, Fragment> parameters;
				
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

				template<class T> T *alloc_var()
				{
					T *result = new (Fragment(*fragment)) T;
					
					result->loc = variable_list.size();
					
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
void *operator new[](size_t bytes, Mirb::Tree::Fragment fragment) throw();
