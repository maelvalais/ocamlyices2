open Yices2
open OUnit2

let bitsize _ =
  let bv1 = Term.Bitvector.zero 5 in
  let i64 = Term.Int.of_int64 64L in

  OUnit2.assert_bool "is_bitvector bv1" (Term.is_bitvector bv1);
  OUnit2.assert_bool "is_bitvector i64" (not (Term.is_bitvector i64));

  OUnit2.assert_equal 5 (Term.bitsize bv1);
  OUnit2.assert_equal 5 (Type.bitsize (Type.of_term bv1));

  begin
    try
      let _ = Term.bitsize i64 in
      OUnit2.assert_failure "should have raise exception YicesError"
    with YicesError (Error.BITVECTOR_REQUIRED, _) ->
      ()
    |  YicesError (_, r) ->
      prerr_endline r.Error.name;
      OUnit2.assert_failure "the exception raised should be of code BITVECTOR_REQUIRED"
  end;

  begin
    try
      let _ = Type.bitsize (Type.of_term i64) in
      OUnit2.assert_failure "should have raise exception YicesError"
    with YicesError (code, r) ->
      OUnit2.assert_equal ~printer:(fun v->v) r.Error.name "BVTYPE_REQUIRED"
  end