#include "tree.hpp"

namespace Mirb
{
	namespace Tree
	{
		Scope::Scope(Fragment &fragment, Scope *parent, Type type) : fragment(&fragment), type(type), parent(parent), variables(2, &fragment), referenced_scopes(fragment), variable_list(2, fragment), zsupers(fragment)
		{
			require_exceptions = false;
			break_targets = 0;
			can_break = false;
			
			heap_vars = 0;
			
			block_parameter = 0;
			super_module_var = 0;
			super_name_var = 0;
			
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
			for(auto i = owner->parameters.begin(); i != owner->parameters.end(); ++i)
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
				var->index = owner->heap_vars++;
				var->owner = owner;
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
			return (NamedVariable **)fragment->alloc(sizeof(NamedVariable *) * entries);
		}
		
		Fragment::Fragment(Fragment *parent, size_t chunk_size) : fragments(*this), chunk_size(chunk_size)
		{
			if(parent)
				parent->fragments.push(this);
			
			current = Chunk::create(chunk_size);
			chunks.append(current);
		}
		
		void *Fragment::realloc(void *mem, size_t old_size, size_t new_size)
		{
			void *result = alloc(new_size);
			
			memcpy(result, mem, old_size);
			
			return result;
		}
		
		void *Fragment::alloc(size_t bytes)
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
			uint8_t *memory = (uint8_t *)malloc(sizeof(Chunk) + size);
			
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
	return fragment->alloc(bytes);
}

void operator delete(void *, Mirb::Tree::Fragment *fragment) throw()
{
}

void *operator new[](size_t bytes, Mirb::Tree::Fragment *fragment) throw()
{
	return fragment->alloc(bytes);
}

void operator delete[](void *, Mirb::Tree::Fragment *fragment) throw()
{
}
