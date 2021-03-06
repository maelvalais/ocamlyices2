/*
 * The Yices SMT Solver. Copyright 2014 SRI International.
 *
 * This program may only be used subject to the noncommercial end user
 * license agreement which is downloadable along with this program.
 */

/*
 * Table for hash-consing of power products
 */

#include <assert.h>

#include "terms/pprod_table.h"
#include "utils/hash_functions.h"
#include "utils/memalloc.h"



/*
 * Initialization: create an empty table.
 * - n = initial size. If n=0, the default is used.
 */
void init_pprod_table(pprod_table_t *table, uint32_t n) {
  if (n == 0) {
    n = PPROD_TABLE_DEF_SIZE;
  }
  if (n >= PPROD_TABLE_MAX_SIZE) {
    out_of_memory();
  }

  table->data = (pprod_t **) safe_malloc(n * sizeof(pprod_t *));
  table->mark = allocate_bitvector(n);
  table->size = n;
  table->nelems = 0;
  table->free_idx = -1;

  init_int_htbl(&table->htbl, 0); // default size
  init_pp_buffer(&table->buffer, 10);
}



/*
 * Extend the table: make it 50% larger
 */
static void extend_pprod_table(pprod_table_t *table) {
  uint32_t n;

  n = table->size + 1;
  n += n >> 1;
  if (n >= PPROD_TABLE_MAX_SIZE) {
    out_of_memory();
  }

  table->data = (pprod_t **) safe_realloc(table->data, n * sizeof(pprod_t *));
  table->mark = extend_bitvector(table->mark, n);
  table->size = n;
}


/*
 * Empty the table
 */
void reset_pprod_table(pprod_table_t *table) {
  table->nelems = 0;
  table->free_idx = -1;
  reset_int_htbl(&table->htbl);
  pp_buffer_reset(&table->buffer);
}


/*
 * Delete the table and its content
 */
void delete_pprod_table(pprod_table_t *table) {
  pprod_t *p;
  uint32_t i, n;

  n = table->nelems;
  for (i=0; i<n; i++) {
    p = table->data[i];
    if (! has_int_tag(p)) {
      safe_free(p);
    }
  }
  safe_free(table->data);
  delete_bitvector(table->mark);
  table->data = NULL;
  table->mark = NULL;

  delete_int_htbl(&table->htbl);
  delete_pp_buffer(&table->buffer);
}



/*
 * Allocate an index i such that data[i] is empty
 * - clear mark[i]
 */
static int32_t allocate_pprod_id(pprod_table_t *table) {
  int32_t i;

  i = table->free_idx;
  if (i >= 0) {
    assert(i < table->nelems);
    table->free_idx = untag_i32(table->data[i]);
  } else {
    i = table->nelems;
    table->nelems ++;
    if (i == table->size) {
      extend_pprod_table(table);
    }
    assert(i < table->size);
  }

  clr_bit(table->mark, i);

  return i;
}



/*
 * Erase descriptor i:
 * - free prod[i] and add i to the free list
 */
static void erase_pprod_id(pprod_table_t *table, int32_t i) {
  assert(0 <= i && i < table->nelems && !has_int_tag(table->data[i]));

  safe_free(table->data[i]);
  table->data[i] = tag_i32(table->free_idx);
  table->free_idx = i;
}



/*
 * HASH CONSING
 */

/*
 * Object for hash consing from an array of pairs (variable, exponent).
 * - len = length of the array
 */
typedef struct pprod_hobj_s {
  int_hobj_t m;
  pprod_table_t *tbl;
  varexp_t *array;
  uint32_t len;
} pprod_hobj_t;


/*
 * Hash function
 */
static uint32_t hash_varexp_array(varexp_t *a, uint32_t n) {
  assert(n <= UINT32_MAX/2);
  return jenkins_hash_intarray((int32_t *) a, 2 * n);
}

static uint32_t hash_pprod(pprod_hobj_t *o) {
  return hash_varexp_array(o->array, o->len);
}

/*
 * Equality test
 */
static bool eq_pprod(pprod_hobj_t *o, int32_t i) {
  pprod_table_t *table;
  pprod_t *p;
  uint32_t n;

  table = o->tbl;
  assert(0 <= i && i < table->nelems && !has_int_tag(table->data[i]));

  p = table->data[i];
  n = o->len;
  return (n == p->len) && varexp_array_equal(o->array, p->prod, n);
}


/*
 * Constructor
 */
static int32_t build_pprod(pprod_hobj_t *o) {
  pprod_table_t *table;
  int32_t i;

  table = o->tbl;
  i = allocate_pprod_id(table);
  table->data[i] = make_pprod(o->array, o->len);

  return i;
}


/*
 * Global hash object
 */
static pprod_hobj_t pprod_hobj = {
  { (hobj_hash_t) hash_pprod, (hobj_eq_t) eq_pprod, (hobj_build_t) build_pprod },
  NULL,
  NULL,
  0,
};



/*
 * Hash consing function:
 * - a must be normalized, non empty, and not equal to (x^1)
 * - n = size of array a
 */
static pprod_t *get_pprod(pprod_table_t *table, varexp_t *a, uint32_t n) {
  int32_t i;

  assert(n > 1 || (n == 1 && a[0].exp > 1));
  pprod_hobj.tbl = table;
  pprod_hobj.array = a;
  pprod_hobj.len = n;

  i = int_htbl_get_obj(&table->htbl, &pprod_hobj.m);

  return table->data[i];
}



/*
 * Top-level constructor:
 * - check whether a is empty or equal to (x^1) if not use hash consing.
 * - a must be normalized
 */
pprod_t *pprod_from_array(pprod_table_t *table, varexp_t *a, uint32_t n) {
  if (n == 0) {
    return empty_pp;
  }
  if (n == 1 && a[0].exp == 1) {
    return var_pp(a[0].var);
  }

  return get_pprod(table, a, n);
}



/*
 * Product (p1 * p2)
 */
pprod_t *pprod_mul(pprod_table_t *table, pprod_t *p1, pprod_t *p2) {
  pp_buffer_t *b;

  b = &table->buffer;
  pp_buffer_set_pprod(b, p1);
  pp_buffer_mul_pprod(b, p2);

  return pprod_from_array(table, b->prod, b->len);
}


/*
 * Exponentiation: (p ^ d)
 */
pprod_t *pprod_exp(pprod_table_t *table, pprod_t *p, uint32_t d) {
  pp_buffer_t *b;

  b = &table->buffer;
  pp_buffer_set_pprod(b, p);
  pp_buffer_exponentiate(b, d);

  return pprod_from_array(table, b->prod, b->len);
}


/*
 * Variable power: (x ^ d)
 */
pprod_t *pprod_varexp(pprod_table_t *table, int32_t x, uint32_t d) {
  pp_buffer_t *b;

  b = &table->buffer;
  pp_buffer_set_varexp(b, x, d);
  pp_buffer_normalize(b);

  return pprod_from_array(table, b->prod, b->len);
}


/*
 * Find the index of p in table
 * - return -1 if p is not in the table
 */
static int32_t find_pprod_id(pprod_table_t *table, pprod_t *p) {
  assert(p != empty_pp && p != end_pp && !pp_is_var(p));

  // search for p's index using the hash table
  pprod_hobj.tbl = table;
  pprod_hobj.array = p->prod;
  pprod_hobj.len = p->len;

  return int_htbl_find_obj(&table->htbl, &pprod_hobj.m);
}

/*
 * Remove p from the table and free the corresponding pprod_t object.
 * - p must be present in the table (and must be distinct from end_pp,
 *   empty_pp, or any tagged variable).
 */
void delete_pprod(pprod_table_t *table, pprod_t *p) {
  int32_t i;
  uint32_t h;

  assert(p != empty_pp && p != end_pp && !pp_is_var(p));
  assert(p->len > 1 || (p->len == 1 && p->prod[0].exp > 1));

  /*
   * This is suboptimal but that should not matter too much.
   * We search for p's index i in the hash table then
   * we search the hash table again to delete the record (h, i).
   */
  i = find_pprod_id(table, p);
  assert(i >= 0 && table->data[i] == p);

  // keep h = hash code of p
  h = hash_varexp_array(p->prod, p->len);

  // delete p and recycle index i
  erase_pprod_id(table, i);  // this deletes p

  // remove the record [h, i] from the hash table
  int_htbl_erase_record(&table->htbl, h, i);
}


/*
 * Set the garbage collection mark for p
 * - p must be present in the table (and must be distinct from end_pp,
 *   empty_pp, or any tagged variable).
 * - once p is marked it will not be deleted on the next call to pprod_table_gc
 */
void pprod_table_set_gc_mark(pprod_table_t *table, pprod_t *p) {
  int32_t i;

  i = find_pprod_id(table, p);
  assert(i >= 0 && table->data[i] == p);
  set_bit(table->mark, i);
}


/*
 * Garbage collection: delete all unmarked products
 * clear all the marks
 */
void pprod_table_gc(pprod_table_t *table) {
  pprod_t *p;
  uint32_t i, n, h;

  n = table->nelems;
  for (i=0; i<n; i++) {
    if (! tst_bit(table->mark, i)) {
      // i is not marked
      p = table->data[i];
      if (!has_int_tag(p)) {
        // not already deleted
        h = hash_varexp_array(p->prod, p->len);
        erase_pprod_id(table, i);
        int_htbl_erase_record(&table->htbl, h, i);
      }
    }
  }

  // clear all the marks
  clear_bitvector(table->mark, table->size);
}
