(set-logic QF_LIA)
(declare-fun x () Int)
(assert ((_ divisible 3) x))
(assert (> x 0))
(assert (< x 6))
(check-sat)
(get-model)
(get-value (x (div x 3) (mod x 3) ((_ divisible 3) x)))
