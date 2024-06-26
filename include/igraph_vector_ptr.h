/* -*- mode: C -*-  */
/*
   IGraph library.
   Copyright (C) 2009-2012  Gabor Csardi <csardi.gabor@gmail.com>
   334 Harvard street, Cambridge, MA 02139 USA

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA

*/

#ifndef IGRAPH_VECTOR_PTR_H
#define IGRAPH_VECTOR_PTR_H

#include "igraph_decls.h"
#include "igraph_error.h"
#include "igraph_vector.h"

__BEGIN_DECLS

/* -------------------------------------------------- */
/* Flexible vector, storing pointers                  */
/* -------------------------------------------------- */

/**
 * Vector, storing pointers efficiently
 * \ingroup internal
 *
 */
typedef struct s_vector_ptr {
    void** stor_begin;
    void** stor_end;
    void** end;
    igraph_finally_func_t* item_destructor;
} igraph_vector_ptr_t;

#define IGRAPH_VECTOR_PTR_NULL { 0,0,0,0 }
#define IGRAPH_VECTOR_PTR_INIT_FINALLY(v, size) \
    do { IGRAPH_CHECK(igraph_vector_ptr_init(v, size)); \
         IGRAPH_FINALLY(igraph_vector_ptr_destroy, v); } while (0)

IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_init(igraph_vector_ptr_t* v, igraph_integer_t size);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_init_array(igraph_vector_ptr_t* v, void *const *data, igraph_integer_t length);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_init_copy(igraph_vector_ptr_t *to, const igraph_vector_ptr_t *from);
IGRAPH_EXPORT const igraph_vector_ptr_t *igraph_vector_ptr_view (const igraph_vector_ptr_t *v,
                                                                 void *const *data, igraph_integer_t length);
IGRAPH_EXPORT void igraph_vector_ptr_destroy(igraph_vector_ptr_t* v);
IGRAPH_EXPORT void igraph_vector_ptr_free_all(igraph_vector_ptr_t* v);
IGRAPH_EXPORT void igraph_vector_ptr_destroy_all(igraph_vector_ptr_t* v);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_reserve(igraph_vector_ptr_t* v, igraph_integer_t capacity);
IGRAPH_EXPORT IGRAPH_FUNCATTR_PURE igraph_bool_t igraph_vector_ptr_empty(const igraph_vector_ptr_t* v);
IGRAPH_EXPORT IGRAPH_FUNCATTR_PURE igraph_integer_t igraph_vector_ptr_size(const igraph_vector_ptr_t* v);
IGRAPH_EXPORT void igraph_vector_ptr_clear(igraph_vector_ptr_t* v);
IGRAPH_EXPORT void igraph_vector_ptr_null(igraph_vector_ptr_t* v);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_push_back(igraph_vector_ptr_t* v, void* e);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_append(igraph_vector_ptr_t *to,
                                                      const igraph_vector_ptr_t *from);
IGRAPH_EXPORT void *igraph_vector_ptr_pop_back(igraph_vector_ptr_t *v);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_insert(igraph_vector_ptr_t *v, igraph_integer_t pos, void* e);
IGRAPH_EXPORT IGRAPH_DEPRECATED void* igraph_vector_ptr_e(const igraph_vector_ptr_t* v, igraph_integer_t pos);
IGRAPH_EXPORT IGRAPH_FUNCATTR_PURE void* igraph_vector_ptr_get(const igraph_vector_ptr_t* v, igraph_integer_t pos);
IGRAPH_EXPORT void igraph_vector_ptr_set(igraph_vector_ptr_t* v, igraph_integer_t pos, void* value);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_resize(igraph_vector_ptr_t* v, igraph_integer_t newsize);
IGRAPH_EXPORT void igraph_vector_ptr_copy_to(const igraph_vector_ptr_t *v, void** to);
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_permute(igraph_vector_ptr_t* v, const igraph_vector_int_t* index);
IGRAPH_EXPORT void igraph_vector_ptr_remove(igraph_vector_ptr_t *v, igraph_integer_t pos);
IGRAPH_EXPORT void igraph_vector_ptr_sort(igraph_vector_ptr_t *v, int(*compar)(const void*, const void*));
IGRAPH_EXPORT igraph_error_t igraph_vector_ptr_sort_ind(
        igraph_vector_ptr_t *v, igraph_vector_int_t *inds, int(*compar)(const void*, const void*));

IGRAPH_EXPORT IGRAPH_FUNCATTR_PURE igraph_finally_func_t* igraph_vector_ptr_get_item_destructor(const igraph_vector_ptr_t *v);
IGRAPH_EXPORT igraph_finally_func_t* igraph_vector_ptr_set_item_destructor(igraph_vector_ptr_t *v,
                                                                           igraph_finally_func_t *func);

IGRAPH_EXPORT IGRAPH_DEPRECATED igraph_error_t igraph_vector_ptr_copy(igraph_vector_ptr_t *to, const igraph_vector_ptr_t *from);

/**
 * \define IGRAPH_VECTOR_PTR_SET_ITEM_DESTRUCTOR
 * \brief Sets the item destructor for this pointer vector (macro version).
 *
 * This macro is expanded to \ref igraph_vector_ptr_set_item_destructor(), the
 * only difference is that the second argument is automatically cast to an
 * \c igraph_finally_func_t*. The cast is necessary in most cases as the
 * destructor functions we use (such as \ref igraph_vector_destroy()) take a
 * pointer to some concrete igraph data type, while \c igraph_finally_func_t
 * expects \c void*
 */
#define IGRAPH_VECTOR_PTR_SET_ITEM_DESTRUCTOR(v, func) \
    igraph_vector_ptr_set_item_destructor((v), (igraph_finally_func_t*)(func))

__END_DECLS

#endif
