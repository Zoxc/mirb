#pragma once

/*
 * Based on code by Attractive Chaos (0.2.2)
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef size_t hash_iter_t;

#define __ac_HASH_PRIME_SIZE 32
static const size_t __ac_prime_list[__ac_HASH_PRIME_SIZE] =
{
  0ul,          3ul,          11ul,         23ul,         53ul,
  97ul,         193ul,        389ul,        769ul,        1543ul,
  3079ul,       6151ul,       12289ul,      24593ul,      49157ul,
  98317ul,      196613ul,     393241ul,     786433ul,     1572869ul,
  3145739ul,    6291469ul,    12582917ul,   25165843ul,   50331653ul,
  100663319ul,  201326611ul,  402653189ul,  805306457ul,  1610612741ul,
  3221225473ul, 4294967291ul
};

#define __ac_isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define __ac_isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define __ac_iseither(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&3)
#define __ac_set_isdel_false(flag, i) (flag[i>>4]&=~(1ul<<((i&0xfU)<<1)))
#define __ac_set_isempty_false(flag, i) (flag[i>>4]&=~(2ul<<((i&0xfU)<<1)))
#define __ac_set_isboth_false(flag, i) (flag[i>>4]&=~(3ul<<((i&0xfU)<<1)))
#define __ac_set_isdel_true(flag, i) (flag[i>>4]|=1ul<<((i&0xfU)<<1))

static const double __ac_HASH_UPPER = 0.77;

#define HASH_INIT(name, key_t, val_t, is_map, __hash_func, __hash_equal, fields, init_args, init_fields, malloc_bootstrap, malloc, realloc, free) \
	typedef struct {													\
		size_t n_buckets, size, n_occupied, upper_bound;				\
		uint32_t *flags;												\
		key_t *keys;													\
		val_t *vals;													\
		fields \
	} hash_##name##_t;													\
	static inline hash_##name##_t *hash_init_##name(init_args) {						\
		hash_##name##_t *h = (hash_##name##_t *)malloc_bootstrap(sizeof(hash_##name##_t)); \
		memset(h, 0, sizeof(hash_##name##_t)); \
		init_fields \
		return h;	\
	}																	\
	static inline void hash_destroy_##name(hash_##name##_t *h)				\
	{																	\
		if (h) {														\
			free(h->keys); free(h->flags);								\
			free(h->vals);												\
			free(h);													\
		}																\
	}																	\
	static inline void hash_clear_##name(hash_##name##_t *h)				\
	{																	\
		if (h && h->flags) { \
			memset(h->flags, 0xaa, ((h->n_buckets>>4) + 1) * sizeof(uint32_t)); \
			h->size = h->n_occupied = 0;								\
		}																\
	}																	\
	static inline size_t hash_get_##name(hash_##name##_t *h, key_t key)	\
	{																	\
		if (h->n_buckets) {												\
			size_t inc, k, i, last;									\
			k = __hash_func(key); i = k % h->n_buckets;					\
			inc = 1 + k % (h->n_buckets - 1); last = i;					\
			while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !__hash_equal(h->keys[i], key))) { \
				if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets; \
				else i += inc;											\
				if (i == last) return h->n_buckets;						\
			}															\
			return __ac_iseither(h->flags, i)? h->n_buckets : i;			\
		} else return 0;												\
	}																	\
	static inline void hash_resize_##name(hash_##name##_t *h, size_t new_n_buckets) \
	{																	\
		uint32_t *new_flags = 0;										\
		size_t j = 1;													\
		{																\
			size_t t = __ac_HASH_PRIME_SIZE - 1;						\
			while (__ac_prime_list[t] > new_n_buckets) --t;				\
			new_n_buckets = __ac_prime_list[t+1];						\
			if (h->size >= (size_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0;	\
			else {														\
				new_flags = (uint32_t*)malloc(((new_n_buckets>>4) + 1) * sizeof(uint32_t));	\
				memset(new_flags, 0xaa, ((new_n_buckets>>4) + 1) * sizeof(uint32_t)); \
				if (h->n_buckets < new_n_buckets) {						\
					h->keys = (key_t*)realloc(h->keys, h->n_buckets * sizeof(key_t), new_n_buckets * sizeof(key_t)); \
					if (is_map)										\
						h->vals = (val_t*)realloc(h->vals, h->n_buckets * sizeof(val_t), new_n_buckets * sizeof(val_t)); \
				}														\
			}															\
		}																\
		if (j) {														\
			for (j = 0; j != h->n_buckets; ++j) {						\
				if (__ac_iseither(h->flags, j) == 0) {					\
					key_t key = h->keys[j];							\
					val_t val;										\
					if (is_map) val = h->vals[j];					\
					__ac_set_isdel_true(h->flags, j);					\
					while (1) {											\
						size_t inc, k, i;								\
						k = __hash_func(key);							\
						i = k % new_n_buckets;							\
						inc = 1 + k % (new_n_buckets - 1);				\
						while (!__ac_isempty(new_flags, i)) {			\
							if (i + inc >= new_n_buckets) i = i + inc - new_n_buckets; \
							else i += inc;								\
						}												\
						__ac_set_isempty_false(new_flags, i);			\
						if (i < h->n_buckets && __ac_iseither(h->flags, i) == 0) { \
							{ key_t tmp = h->keys[i]; h->keys[i] = key; key = tmp; } \
							if (is_map) { val_t tmp = h->vals[i]; h->vals[i] = val; val = tmp; } \
							__ac_set_isdel_true(h->flags, i);			\
						} else {										\
							h->keys[i] = key;							\
							if (is_map) h->vals[i] = val;			\
							break;										\
						}												\
					}													\
				}														\
			}															\
			if (h->n_buckets > new_n_buckets) {							\
				h->keys = (key_t*)realloc(h->keys, h->n_buckets * sizeof(key_t), new_n_buckets * sizeof(key_t)); \
				if (is_map)											\
					h->vals = (val_t*)realloc(h->vals, h->n_buckets * sizeof(val_t), new_n_buckets * sizeof(val_t)); \
			}															\
			free(h->flags);												\
			h->flags = new_flags;										\
			h->n_buckets = new_n_buckets;								\
			h->n_occupied = h->size;									\
			h->upper_bound = (size_t)(h->n_buckets * __ac_HASH_UPPER + 0.5); \
		}																\
	}																	\
	static inline size_t hash_put_##name(hash_##name##_t *h, key_t key, int *ret) \
	{																	\
		size_t x;														\
		if (h->n_occupied >= h->upper_bound) {							\
			if (h->n_buckets > (h->size<<1)) hash_resize_##name(h, h->n_buckets - 1); \
			else hash_resize_##name(h, h->n_buckets + 1);					\
		}																\
		{																\
			size_t inc, k, i, site, last;								\
			x = site = h->n_buckets; k = __hash_func(key); i = k % h->n_buckets; \
			if (__ac_isempty(h->flags, i)) x = i;						\
			else {														\
				inc = 1 + k % (h->n_buckets - 1); last = i;				\
				while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !__hash_equal(h->keys[i], key))) { \
					if (__ac_isdel(h->flags, i)) site = i;				\
					if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets; \
					else i += inc;										\
					if (i == last) { x = site; break; }					\
				}														\
				if (x == h->n_buckets) {								\
					if (__ac_isempty(h->flags, i) && site != h->n_buckets) x = site; \
					else x = i;											\
				}														\
			}															\
		}																\
		if (__ac_isempty(h->flags, x)) {								\
			h->keys[x] = key;											\
			__ac_set_isboth_false(h->flags, x);							\
			++h->size; ++h->n_occupied;									\
			*ret = 1;													\
		} else if (__ac_isdel(h->flags, x)) {							\
			h->keys[x] = key;											\
			__ac_set_isboth_false(h->flags, x);							\
			++h->size;													\
			*ret = 2;													\
		} else *ret = 0;												\
		return x;														\
	}																	\
	static inline void hash_del_##name(hash_##name##_t *h, size_t x)		\
	{																	\
		if (x != h->n_buckets && !__ac_iseither(h->flags, x)) {			\
			__ac_set_isdel_true(h->flags, x);							\
			--h->size;													\
		}																\
	}


#define hash_int_hash_func(key) (size_t)(key)
#define hash_int_hash_equal(a, b) ((a) == (b))

static inline size_t __ac_X31_hash_string(const char *s)
{
	size_t h = *s;
	if (h) for (++s ; *s; ++s) h = (h << 5) - h + *s;
	return h;
}
#define hash_str_hash_func(key) __ac_X31_hash_string(key)
#define hash_str_hash_equal(a, b) (strcmp(a, b) == 0)

/* Other necessary macros... */

#define hash_t(name) hash_##name##_t

#define hash_init(name, ...) hash_init_##name(__VA_ARGS__)
#define hash_destroy(name, ...) hash_destroy_##name(__VA_ARGS__)
#define hash_clear(name, ...) hash_clear_##name(__VA_ARGS__)
#define hash_resize(name, ...) hash_resize_##name(__VA_ARGS__)
#define hash_put(name, ...) hash_put_##name(__VA_ARGS__)
#define hash_get(name, ...) hash_get_##name(__VA_ARGS__)
#define hash_del(name, ...) hash_del_##name(__VA_ARGS__)

#define hash_exist(h, x) (!__ac_iseither((h)->flags, (x)))
#define hash_key(h, x) ((h)->keys[x])
#define hash_val(h, x) ((h)->vals[x])
#define hash_value(h, x) ((h)->vals[x])
#define hash_begin(h) (size_t)(0)
#define hash_end(h) ((h)->n_buckets)
#define hash_size(h) ((h)->size)
#define hash_n_buckets(h) ((h)->n_buckets)

#define hash_default_malloc(size) malloc(size)
#define hash_default_realloc(old, old_size, new_size) realloc(old, new_size)
#define hash_default_free(obj) free(obj)

/* More convenient interfaces */

#define HASH_DEFAULT(name, key_t, val_t)								\
	HASH_INIT(name, key_t, val_t, 1, hash_int_hash_func, hash_int_hash_equal, , , , hash_default_malloc, hash_default_malloc, hash_default_realloc, hash_default_free)

#define HASH_DEFAULT_STR(name, val_t)								\
	HASH_INIT(name, const char *, val_t, 1, hash_str_hash_func, hash_str_hash_equal, , , , hash_default_malloc, hash_default_malloc, hash_default_realloc, hash_default_free)
