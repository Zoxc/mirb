#include "tree.hpp"

namespace Mirb
{
	namespace Tree
	{
		void *FragmentVector::alloc(Fragment *storage, size_t bytes)
		{
			return storage->allocate(bytes);
		}
		
		void *FragmentVector::realloc(Fragment *storage, void *table, size_t old, size_t bytes)
		{
			void *result = storage->allocate(bytes);
			
			memcpy(result, table, old);
			
			return result;
		}
		
		void FragmentVector::free(Fragment *storage, void *table)
		{
		}
		
		Scope::Scope(Fragment *fragment, Scope *parent, Type type) : fragment(fragment), type(type), parent(parent), variables(2, fragment), referenced_scopes(fragment)
		{
			owner = parent ? parent->owner : 0;
		}
	
		void Scope::require_scope(Scope *scope)
		{
			if(referenced_scopes.find(scope))
				return;
			
			referenced_scopes.push(scope);
		}
		

		void Scope::require_args(Scope *owner)
		{
			for(auto i = owner->parameters.begin(); i; ++i)
			{
				require_var(owner, *i);
			}
			
			if(owner->block_parameter)
				require_var(owner, owner->block_parameter);
		}

		void Scope::require_var(Scope *owner, Variable *var)
		{
			if(var->type != Variable::Heap)
			{
				var->type = Variable::Heap;
				var->index = owner->var_count[Variable::Heap]++;
				var->owner = owner;
				
				owner->heap_vars = true;
			}
			
			/*
			 * Make sure the block owning the varaible is required by the current block and parents.
			 */
			
			Scope *current = this;
			
			while(current != owner)
			{
				current->require_scope(owner);
				
				current = current->parent;
			}
		}

		Scope *Scope::defined(Symbol *name, bool recursive)
		{
			if(variables.get(name))
				return this;
			
			Scope *current = this;
			
			if(recursive)
			{
				while(current->type == Scope::Closure)
				{
					current = current->parent;
					
					if(current->variables.get(name))
						return current;
				}
			}
			
			return 0;
		}
		
		NamedVariable **VariableMapFunctions::alloc(Fragment *fragment, size_t entries)
		{
			return (NamedVariable **)fragment->allocate(sizeof(NamedVariable *) * entries);
		}

		Fragment::Fragment(Fragment *parent, size_t chunk_size) : chunk_size(chunk_size)
		{
			if(parent)
				parent->fragments.append(this);
			
			current = Chunk::create(chunk_size);
			chunks.append(current);
		}
		
		void *Fragment::allocate(size_t bytes)
		{
			void *result = current->allocate(bytes);
			
			if(!result)
			{
				Chunk *chunk;
				
				if(bytes > Chunk::allocation_limit)
				{
					chunk = Chunk::create(bytes);
				}
				else
				{
					chunk = Chunk::create(chunk_size);
					current = chunk;
				}
				
				chunks.append(chunk);
				
				result = chunk->allocate(bytes);
				
				assert(result);
			}
			
			return result;
		}
		
		Chunk *Chunk::create(size_t size)
		{
			uint8_t *memory = (unsigned char *)malloc(sizeof(Chunk) + size);
			
			assert(memory);
			
			Chunk *result = new (memory) Chunk;
			
			result->current = memory + sizeof(Chunk);
			result->end = result->current + size;
			
			return result;
		}
		
		void *Chunk::allocate(size_t bytes)
		{
			uint8_t *result;
			
			#ifdef VALGRIND
				result = (uint8_t *)malloc(bytes);
				
				assert(result);
			#else
				result = current;
				
				uint8_t *next = result + bytes;
				
				if(next >= end)
					return 0;
				
				current = next;
			#endif
			
			return (void *)result;
		}
	};
};

void *operator new(size_t bytes, Mirb::Tree::Fragment *fragment) throw()
{
	return fragment->allocate(bytes);
}

void operator delete(void *, Mirb::Tree::Fragment *fragment) throw()
{
}

void *operator new[](size_t bytes, Mirb::Tree::Fragment *fragment) throw()
{
	return fragment->allocate(bytes);
}

void operator delete[](void *, Mirb::Tree::Fragment *fragment) throw()
{
}
