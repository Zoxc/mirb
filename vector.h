#pragma once

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
 * Convenience macros
 */
#define vec_t(name) struct vec_##name
#define vec_init(name, ...) vec_##name##_init(__VA_ARGS__)
#define vec_destroy(name, ...) vec_##name##_destroy(__VA_ARGS__)
#define vec_push(name, ...) vec_##name##_push(__VA_ARGS__)
#define vec_pop(name, ...) vec_##name##_pop(__VA_ARGS__)
#define vec_mov(name, ...) vec_##name##_mov(__VA_ARGS__)
#define vec_dup(name, ...) vec_##name##_dup(__VA_ARGS__)

#define vec_default_malloc(size) malloc(size)
#define vec_default_realloc(old, old_size, new_size) realloc(old, new_size)
#define vec_default_free(obj) free(obj)
#define VEC_DEFAULT(type, name) \
	VEC_INIT(type, name, , , , , vec_default_malloc, vec_default_realloc, vec_default_free)

/*
 * Default malloc implementation
 */

#define VEC_INIT(type, name, fields, init_args, init_fields, mov_fields, malloc, realloc, free) \
	struct vec_##name \
	{ \
		size_t size; \
		size_t max; \
		type *array; \
		fields \
	}; \
	static inline type vec_##name##_pop(struct vec_##name *vec) \
	{ \
		return vec->array[--(vec->size)]; \
	} \
	static inline void vec_##name##_destroy(struct vec_##name *vec) \
	{ \
		free(vec->array); \
	} \
	static inline void vec_##name##_init(struct vec_##name *vec init_args) \
	{ \
		vec->size = 0; \
		vec->max = 0; \
		vec->array = 0; \
		init_fields \
	} \
	static inline void vec_##name##_mov(struct vec_##name *new_vec, struct vec_##name *vec) \
	{ \
        new_vec->array = vec->array; \
        new_vec->size = vec->size; \
        new_vec->max = vec->max; \
        vec_##name##_init(vec mov_fields);\
	} \
	static inline void vec_##name##_dup(struct vec_##name *vec, struct vec_##name *old_vec) \
	{ \
		size_t bytesize = old_vec->size * sizeof(type); \
		memcpy(vec, old_vec, sizeof(struct vec_##name)); \
		vec->array = (type *)(malloc(bytesize)); \
		memcpy(vec->array, old_vec->array, bytesize); \
	} \
	static inline void vec_##name##_resize(struct vec_##name *vec, size_t new_size) \
	{ \
		vec->array = (type *)(realloc(vec->array, vec->size, sizeof(type) * new_size)); \
		vec->max = new_size; \
	} \
	static inline void vec_##name##_push(struct vec_##name *vec, type value) \
	{ \
		if (vec->size == vec->max) \
		{ \
			vec->max = vec->max ? (vec->max << 1) : 2; \
			vec->array = (type *)(realloc(vec->array, vec->size, sizeof(type) * vec->max)); \
		} \
		vec->array[vec->size++] = value; \
	}

/*
#define kv_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

	#define kv_##name##_pushp(type, v) (((v).n == (v).m)?							\
							   ((v).m = ((v).m? (v).m<<1 : 2),				\
								(v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0)	\
							   : 0), ((v).a + ((v).n++))

	#define kv_##name##_a(type, v, i) ((v).m <= (size_t)(i)?						\
							  ((v).m = (v).n = (i) + 1, kv_roundup32((v).m), \
							   (v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0) \
							  : (v).n <= (size_t)(i)? (v).n = (i)			\
							  : 0), (v).a[(i)]
*/

