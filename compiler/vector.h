#pragma once
#include "allocator.h"

/*
 * Based on code by Attractive Chaos
 */

/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractivechaos@aol.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
 * Implemention using the compiler allocator
 */
#define COMPILER_VEC_INIT(type, name) \
	struct vec_##name \
	{ \
		size_t size; \
		size_t max; \
		type *array; \
		struct allocator *allocator; \
	}; \
	static inline type vec_##name##_pop(struct vec_##name *vec) \
	{ \
		return vec->array[--(vec->size)]; \
	} \
	static inline void vec_##name##_init(struct vec_##name *vec, struct allocator *allocator) \
	{ \
		vec->size = 0; \
		vec->max = 0; \
		vec->array = 0; \
		vec->allocator = allocator; \
	} \
	static inline void vec_##name##_mov(struct vec_##name *new_vec, struct vec_##name *old_vec) \
	{ \
        new_vec->array = old_vec->array; \
        new_vec->size = old_vec->size; \
        new_vec->max = old_vec->max; \
        vec_##name##_init(old_vec, old_vec->allocator);\
	} \
	static inline void vec_##name##_dup(struct vec_##name *new_vec, struct vec_##name *old_vec) \
	{ \
		size_t bytesize = old_vec->size * sizeof(type); \
		new_vec->allocator = old_vec->allocator; \
		new_vec->array = (type *)allocator_alloc(new_vec->allocator, bytesize); \
		new_vec->size = new_vec->max = old_vec->size; \
		memcpy(new_vec->array, old_vec->array, bytesize); \
	} \
	static inline void vec_##name##_resize(struct vec_##name *vec, size_t new_size) \
	{ \
		vec->max = new_size; \
		vec->array = (type *)allocator_realloc(vec->allocator, vec->array, sizeof(type) * vec->size, sizeof(type) * new_size); \
	} \
	static inline void vec_##name##_push(struct vec_##name *vec, type value) \
	{ \
		if (vec->size == vec->max) \
		{ \
			vec->max = vec->max ? (vec->max << 1) : 2; \
			vec->array = (type *)allocator_realloc(vec->allocator, vec->array, sizeof(type) * vec->size, sizeof(type) * vec->max); \
		} \
		vec->array[vec->size++] = value; \
	} \

