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
  An example:

#include "kvec.h"
int main() {
	kvec_t(int) array;
	kv_init(array);
	kv_push(int, array, 10); // append
	kv_a(int, array, 20) = 5; // dynamic
	kv_A(array, 20) = 4; // static
	kv_destroy(array);
	return 0;
}
*/

/*
  2008-09-22 (0.1.0):

	* The initial version.

*/

/*
 * This file contains methods specific for use with the compiler.
 * Modified by Zoxc
 */

#ifndef AC_CP_KVEC_H
#define AC_CP_KVEC_H

#include <stdlib.h>

#define kv_cp_dup(type, new, old, compiler) do { \
        (new).a = (type*)malloc((old).n * sizeof(type)); \
        (new).n = (new).m = (old).n; \
        memcpy((new).a, (old).a, (old).n * sizeof(type)); \
    } while (0)

#define kv_cp_resize(type, v, s, compiler) do { ((v).m = (s), (v).a = (type*)realloc((v).a, sizeof(type) * (v).m)) \
	(type) *vec = (v); \
	size_t nm = (s); \
	vec.a = (type*)allocator_realloc(&compiler->allocator, vec.a, sizeof(type) * vec.m, sizeof(type) * nm); \
	vec.m = nm; \
	} while (0)

#define kv_cp_push(type, v, x, compiler) do {						\
		if ((v).n == (v).m) {										\
			size_t old = sizeof(type) * (v).m; 						\
			(v).m = (v).m? (v).m<<1 : 2;							\
			(v).a = (type*)allocator_realloc(&compiler->allocator, (v).a, old, sizeof(type) * (v).m);	\
		}															\
		(v).a[(v).n++] = (x);										\
	} while (0)

#define kv_cp_pushp(type, v, compiler) ({ \
	if((v).n == (v).m) \							\
	{ \
		size_t old = sizeof(type) * (v).m; \
		(v).m = ((v).m? (v).m<<1 : 2)				\
		(v).a = (type*)allocator_realloc(&compiler->allocator, (v).a, old, sizeof(type) * (v).m)	\
	} \
	(v).a + ((v).n++)})

/* This should not be used by the compiler
#define kv_cp_a(type, v, i, compiler) ((v).m <= (size_t)(i)?						\
						  ((v).m = (v).n = (i) + 1, kv_roundup32((v).m), \
						   (v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0) \
						  : (v).n <= (size_t)(i)? (v).n = (i)			\
						  : 0), (v).a[(i)]
*/
#endif
