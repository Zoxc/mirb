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

typedef size_t hash_hiter_t;

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

#define hash_int_hash_func(key) (uint32_t)(key)
#define hash_int_hash_equal(a, b) (a == b)
#define hash_int64_hash_func(key) (uint32_t)((key)>>33^(key)^(key)<<11)
#define hash_int64_hash_equal(a, b) (a == b)

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

#define hash_init(name) hash_init_##name()
#define hash_destroy(name, h) hash_destroy_##name(h)
#define hash_clear(name, h) hash_clear_##name(h)
#define hash_resize(name, h, s) hash_resize_##name(h, s)
#define hash_put(name, h, k, r) hash_put_##name(h, k, r)
#define hash_get(name, h, k) hash_get_##name(h, k)
#define hash_del(name, h, k) hash_del_##name(h, k)

#define hash_exist(h, x) (!__ac_iseither((h)->flags, (x)))
#define hash_key(h, x) ((h)->keys[x])
#define hash_val(h, x) ((h)->vals[x])
#define hash_value(h, x) ((h)->vals[x])
#define hash_begin(h) (size_t)(0)
#define hash_end(h) ((h)->n_buckets)
#define hash_size(h) ((h)->size)
#define hash_n_buckets(h) ((h)->n_buckets)

/* More conenient interfaces */

#define HASH_SET_INIT_INT(prefix, name)										\
	prefix##HASH_INIT(name, uint32_t, char, 0, hash_int_hash_func, hash_int_hash_equal)

#define HASH_MAP_INIT_INT(prefix, name, val_t)								\
	prefix##HASH_INIT(name, uint32_t, val_t, 1, hash_int_hash_func, hash_int_hash_equal)

#define HASH_SET_INIT_INT64(prefix, name)										\
	prefix##HASH_INIT(name, uint64_t, char, 0, hash_int64_hash_func, hash_int64_hash_equal)

#define HASH_MAP_INIT_INT64(prefix, name, val_t)								\
	prefix##HASH_INIT(name, uint64_t, val_t, 1, hash_int64_hash_func, hash_int64_hash_equal)

#define HASH_SET_INIT_STR(prefix, name)										\
	prefix##HASH_INIT(name, const char *, char, 0, hash_str_hash_func, hash_str_hash_equal)

#define HASH_MAP_INIT_STR(prefix, name, val_t)								\
	prefix##HASH_INIT(name, const char *, val_t, 1, hash_str_hash_func, hash_str_hash_equal)
