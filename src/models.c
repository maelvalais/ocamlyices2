#include "config.h"

#include <gmp.h> /* should be included before yices and zarith */
#define __GMP_H

#include "models.h"

#include <stdlib.h> // for malloc, free
#include <stdint.h> // for (u)int32_t etc.

#include <yices.h>
#include <caml/mlvalues.h>
#include <caml/custom.h> // custom block
#include <caml/memory.h> // CAMLparam etc.
#include <caml/alloc.h> // caml_alloc, _small, _tuple, caml_copy_string/int32/int64/..., etc.
#include <zarith.h>

#include "misc.h"
#include "terms.h"
#include "contexts.h"


static inline context_t *Mdlctx_val_context(value v) {
  return Context_val(Field(v,1));
}

static void _oy_model_finalize(value);
static char _oy_model_id[] = "ocamlyices.model";
static struct custom_operations _oy_model_ops = {
  _oy_model_id,
  &_oy_model_finalize,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default,
  custom_compare_ext_default,
};

static inline value alloc_model_val () {
  return caml_alloc_custom(&_oy_model_ops, sizeof (model_t *), 0, 1);
}

static inline void Store_model_val(value v, model_t *raw) {
  *((model_t **)Data_custom_val(v)) = raw;
}

CAMLprim value ocamlyices_context_get_model(value v_keepsubst,
    value v_context) {
  CAMLparam2(v_keepsubst, v_context);
  CAMLlocal2(v_model, v_res);
  model_t *model;
  context_t *context;
  int32_t keepsubst;

  context = Context_val(v_context);
  keepsubst = (Is_block(v_keepsubst) ? Int_val(Field(v_keepsubst, 0)) : 1);

  COND_MT_START(MTFLAG_GET_MODEL);
  model = yices_get_model(context, keepsubst);
  COND_MT_END(MTFLAG_GET_MODEL);
  if (model == NULL) {
    _oy_error();
  }

  v_model = alloc_model_val();
  Store_model_val(v_model, model);

  v_res = caml_alloc(2, 0);
  Store_field(v_res, 0, v_model);
  Store_field(v_res, 1, v_context);

  CAMLreturn(v_res);
}

static void _oy_model_finalize(value v_model) {
  model_t *model = Model_val(v_model);

  if (model != NULL) {
    yices_free_model(model);
    Store_model_val(v_model, NULL);
  }
}

CAMLprim value ocamlyices_model_get_bool(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  int32_t res, val;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_bool_value(mdl, t, &val);
  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(Val_long(val != 0));
}

CAMLprim value ocamlyices_model_get_int(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  intnat val;
  int32_t res;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  if (sizeof(intnat) == sizeof(int32_t)) {
    int32_t val32;
    res = yices_get_int32_value(mdl, t, &val32);
    val = val32;
  } else {
    int64_t val64;
    res = yices_get_int64_value(mdl, t, &val64);
    val = val64;
  }

  if (res != 0) {
    _oy_error();
  }

  if (val > Max_long || val < Min_long) {
    _oy_binding_overflow_error();
  }

  CAMLreturn(Val_long(val));
}

CAMLprim value ocamlyices_model_get_nativeint(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  intnat val;
  int32_t res;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  if (sizeof(intnat) == sizeof(int32_t)) {
    int32_t val32;
    res = yices_get_int32_value(mdl, t, &val32);
    val = val32;
  } else {
    int64_t val64;
    res = yices_get_int64_value(mdl, t, &val64);
    val = val64;
  }

  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(caml_copy_nativeint(val));
}

CAMLprim value ocamlyices_model_get_int32(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  int32_t res, val;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_int32_value(mdl, t, &val);
  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(caml_copy_int32(val));
}

CAMLprim value ocamlyices_model_get_int64(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  int32_t res;
  int64_t val;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_int64_value(mdl, t, &val);
  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(caml_copy_int64(val));
}

CAMLprim value ocamlyices_model_get_rational_int(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_res);
  model_t *mdl;
  term_t t;
  int32_t res;
  intnat num;
  uintnat den;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  if (sizeof(intnat) == sizeof(int32_t)) {
    int32_t num32;
    uint32_t den32;
    res = yices_get_rational32_value(mdl, t, &num32, &den32);
    num = num32;
    den = den32;
  } else {
    int64_t num64;
    uint64_t den64;
    res = yices_get_rational64_value(mdl, t, &num64, &den64);
    num = num64;
    den = den64;
  }

  if (res != 0) {
    _oy_error();
  }

  if (num > Max_long || num < Min_long) {
    _oy_binding_overflow_error();
  }
  if (den > Max_long) {
    _oy_binding_overflow_error();
  }

  v_res = caml_alloc_tuple(2);
  Store_field(v_res, 0, Val_long(num));
  Store_field(v_res, 1, Val_long((intnat)den));

  CAMLreturn(v_res);
}

CAMLprim value ocamlyices_model_get_rational_nativeint(value v_mdlctx,
    value v_t) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_res);
  model_t *mdl;
  term_t t;
  int32_t res;
  intnat num;
  uintnat den;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  if (sizeof(intnat) == sizeof(int32_t)) {
    int32_t num32;
    uint32_t den32;
    res = yices_get_rational32_value(mdl, t, &num32, &den32);
    num = num32;
    den = den32;
  } else {
    int64_t num64;
    uint64_t den64;
    res = yices_get_rational64_value(mdl, t, &num64, &den64);
    num = num64;
    den = den64;
  }

  if (res != 0) {
    _oy_error();
  }

  if (num > Max_long || num < Min_long) {
    _oy_binding_overflow_error();
  }
  if (den > Max_long) {
    _oy_binding_overflow_error();
  }

  v_res = caml_alloc_tuple(2);
  Store_field(v_res, 0, caml_copy_nativeint(num));
  Store_field(v_res, 1, caml_copy_nativeint((intnat)den));

  CAMLreturn(v_res);
}

CAMLprim value ocamlyices_model_get_rational_int32(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_res);
  model_t *mdl;
  term_t t;
  int32_t res, num;
  uint32_t den;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_rational32_value(mdl, t, &num, &den);
  if (res != 0) {
    _oy_error();
  }

  if (den > INT32_MAX) {
    _oy_binding_overflow_error();
  }

  v_res = caml_alloc_tuple(2);
  Store_field(v_res, 0, caml_copy_int32(num));
  Store_field(v_res, 1, caml_copy_int32((int32_t)den));

  CAMLreturn(v_res);
}

CAMLprim value ocamlyices_model_get_rational_int64(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_res);
  model_t *mdl;
  term_t t;
  int32_t res;
  int64_t num;
  uint64_t den;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_rational64_value(mdl, t, &num, &den);
  if (res != 0) {
    _oy_error();
  }

  if (den > INT64_MAX) {
    _oy_binding_overflow_error();
  }

  v_res = caml_alloc_tuple(2);
  Store_field(v_res, 0, caml_copy_int64(num));
  Store_field(v_res, 1, caml_copy_int64((int64_t)den));

  CAMLreturn(v_res);
}

CAMLprim value ocamlyices_model_get_float(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  int32_t res;
  double val;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_double_value(mdl, t, &val);
  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(caml_copy_double(val));
}

CAMLprim value ocamlyices_model_get_z(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  int32_t res;
  mpz_t val;

  mpz_init(val);

  res = yices_get_mpz_value(Mdlctx_val_model(v_mdlctx), Term_val(v_t), val);
  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(ml_z_from_mpz(val));
}

CAMLprim value ocamlyices_model_get_q(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_res);
  int32_t res;
  mpq_t val;
  mpz_t num, den;

  mpq_init(val);
  mpz_inits(num, den);

  res = yices_get_mpq_value(Mdlctx_val_model(v_mdlctx), Term_val(v_t), val);
  if (res != 0) {
    _oy_error();
  }

  mpq_get_num(num, val);
  mpq_get_den(den, val);

  v_res = caml_alloc(2, 0);
  Store_field(v_res, 0, ml_z_from_mpz(num));
  Store_field(v_res, 1, ml_z_from_mpz(den));

  CAMLreturn(v_res);
}

CAMLprim value ocamlyices_model_get_bitvector(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_bv);
  model_t *mdl;
  term_t t;
  int32_t res, *bv;
  uint32_t n;
  size_t i;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  n = yices_term_bitsize(t);
  if (n <= 0) {
    _oy_error();
  }

  bv = (int32_t *)malloc(sizeof(int32_t[n]));
  if (bv == NULL) {
    _oy_allocation_error();
  }

  res = yices_get_bv_value(mdl, t, bv);

  if (res != 0) {
    free(bv);
    _oy_error();
  }

  v_bv = caml_alloc(n, 0);
  for (i = 0; i < n; i++) {
    Store_field(v_bv, i, Val_long(bv[i] != 0));
  }

  CAMLreturn(v_bv);
}

CAMLprim value ocamlyices_model_get_scalar(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  term_t t;
  int32_t res, val;

  mdl = Mdlctx_val_model(v_mdlctx);
  t = Term_val(v_t);

  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_scalar_value(mdl, t, &val);

  if (res != 0) {
    _oy_error();
  }

  CAMLreturn(Val_long(val));
}

CAMLprim value ocamlyices_model_get_as_term(value v_mdlctx, value v_t) {
  CAMLparam1(v_mdlctx);
  model_t *mdl;
  int32_t res;

  mdl = Mdlctx_val_model(v_mdlctx);
  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  res = yices_get_value_as_term(mdl, Term_val(v_t));
  if (res == NULL_TERM) {
    _oy_error();
  }

  CAMLreturn(Val_term(res));
}

CAMLprim value ocamlyices_model_get_as_terms(value v_mdlctx, value v_ts) {
  CAMLparam1(v_mdlctx);
  CAMLlocal1(v_outts);

  model_t *mdl;
  term_t *ts, *outts;
  uint32_t lts, i;
  int32_t res;

  mdl = Mdlctx_val_model(v_mdlctx);
  if (mdl == NULL) {
    _oy_freed_model_error();
  }

  lts = check_Wosize_val(v_ts);
  ts = _oy_terms_from_values(v_ts, lts);
  outts = (term_t *)malloc(sizeof(term_t[lts]));

  if (ts == NULL || outts == NULL) {
    free(ts);
    free(outts);
    _oy_allocation_error();
  }

  res = yices_term_array_value(mdl, lts, ts, outts);
  free(ts);
  if (res == NULL_TERM) {
    free(outts);
    _oy_error();
  }

  v_outts = caml_alloc(lts, 0);
  for (i = 0; i < lts; i++) {
    Store_field(v_outts, i, outts[i]);
  }
  free(outts);

  CAMLreturn(v_outts);
}

// Pretty printing

struct pp_model_arg {
  model_t *mdl;
  uint32_t width, height, offset;
};

static int _oy_model_pp(FILE *output, void *arg_) {
  struct pp_model_arg *arg = (struct pp_model_arg *)arg_;
  return yices_pp_model(output, arg->mdl, arg->width, arg->height, arg->offset);
}

CAMLprim value ocamlyices_model_print(value v_width_opt, value v_height_opt,
                                      value v_offset_opt, value v_cb, value v_mdl) {
  CAMLparam4(v_width_opt, v_height_opt, v_offset_opt, v_cb);
  model_t *mdl = Mdlctx_val_model(v_mdl);
  uint32_t width = (uint32_t)Long_option_val(v_width_opt, UINT32_MAX);
  uint32_t height = (uint32_t)Long_option_val(v_height_opt, 1);
  uint32_t offset = (uint32_t)Long_option_val(v_offset_opt, 0);
  struct pp_model_arg arg = { mdl, width, height, offset };
  int res = _oy_callback_print(v_cb, &_oy_model_pp, &arg);
  if (res != 0) {
    _oy_error();
  }
  CAMLreturn(Val_unit);
}

