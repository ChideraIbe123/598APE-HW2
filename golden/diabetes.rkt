#lang racket

;;===== Symbolic Regression Benchmark =====

(provide best-program-1 best-program-2)
;;Loading dataset...
;;Dataset dimensions: 442 samples x 10 features
;;Training symbolic regressor with 512 population size and 16 generations
;;Best program 1 details:
;;- Length: 91 nodes
;;- Depth: 12
;;- Raw fitness: 6302.75
;;- Test MSE: 7468.95
(define best-program-1 (lambda (X)
  (define (div a b) (if (< (abs b) 0.001) 1.0 (/ a b)))
  (define (fdim a b) (max (- a b) 0.0))
  (define (gt a b) (if (> a b) 1.0 0.0))
  (define (acosh a)
    (log (+ a (* (sqrt (- a 1.0)) (sqrt (+ a 1.0))))))
  (define (asinh a)
    (let* ([v (abs a)] [r (log (+ v (sqrt (+ (* v v) 1.0))))])
      (if (< a 0.0) (- r) r)))
  (define (atanh a) (* 0.5 (log (/ (+ 1.0 a) (- 1.0 a)))))
  (define (cbrt a)
    (let ([r (expt (abs a) (/ 1.0 3.0))])
      (if (< a 0.0) (- r) r)))
  (define (cube a) (* a a a))
  (define (inv a) (if (< (abs a) 0.001) 0.0 (/ 1.0 a)))
  (define (rcbrt a) (/ 1.0 (cbrt a)))
  (define (rsqrt a) (/ 1.0 (sqrt (abs a))))
  (define (gp-log a) (if (< (abs a) 0.001) 0.0 (log (abs a))))
  (define (gp-sqrt a) (sqrt (abs a)))
  (*  (-  (-  (+  (cos  (+  (cos  (-  (gp-log  (-  (list-ref X 0) (abs  (list-ref X 8)) ) )  (-  (-  (list-ref X 1) (list-ref X 8))  (sin  (list-ref X 7)) ) ) )  (sin  (cos  (cos  (*  (sin  (cos  (list-ref X 8)) )  (gp-log  (-  (list-ref X 6) (list-ref X 5)) ) ) ) ) ) ) )  (cos  (*  (list-ref X 7) (list-ref X 7)) ) )  (*  (sin  (cos  (list-ref X 8)) )  (gp-log  (-  (list-ref X 6) (list-ref X 5)) ) ) )  (-  (-  (cos  (fdim  (list-ref X 2) (list-ref X 9)) )  (-  (cos  (fdim  (list-ref X 8) (list-ref X 6)) )  (gp-log  (list-ref X 0)) ) )  (*  -0.960798442 (list-ref X 5)) ) )  (+  (fdim  (-  (sin  (cos  (+  (list-ref X 5) (list-ref X 0)) ) )  (cos  (cos  (list-ref X 4)) ) )  (+  (gp-log  (list-ref X 1))  (cos  (gp-log  (list-ref X 9)) ) ) )  (+  (-  (fdim  (abs  (list-ref X 3))  (gp-log  (*  (list-ref X 1) (list-ref X 1)) ) )  (gp-log  (*  (list-ref X 1) (list-ref X 1)) ) )  (abs  (cos  (list-ref X 6)) ) ) ) ) ))
;;Best program 2 details:
;;- Length: 199 nodes
;;- Depth: 18
;;- Raw fitness: 6408.27
;;- Test MSE: 7217.17
(define best-program-2 (lambda (X)
  (define (div a b) (if (< (abs b) 0.001) 1.0 (/ a b)))
  (define (fdim a b) (max (- a b) 0.0))
  (define (gt a b) (if (> a b) 1.0 0.0))
  (define (acosh a)
    (log (+ a (* (sqrt (- a 1.0)) (sqrt (+ a 1.0))))))
  (define (asinh a)
    (let* ([v (abs a)] [r (log (+ v (sqrt (+ (* v v) 1.0))))])
      (if (< a 0.0) (- r) r)))
  (define (atanh a) (* 0.5 (log (/ (+ 1.0 a) (- 1.0 a)))))
  (define (cbrt a)
    (let ([r (expt (abs a) (/ 1.0 3.0))])
      (if (< a 0.0) (- r) r)))
  (define (cube a) (* a a a))
  (define (inv a) (if (< (abs a) 0.001) 0.0 (/ 1.0 a)))
  (define (rcbrt a) (/ 1.0 (cbrt a)))
  (define (rsqrt a) (/ 1.0 (sqrt (abs a))))
  (define (gp-log a) (if (< (abs a) 0.001) 0.0 (log (abs a))))
  (define (gp-sqrt a) (sqrt (abs a)))
  (*  (-  (-  (+  (cos  (*  (list-ref X 7) (list-ref X 7)) )  (cos  (*  (list-ref X 7) (list-ref X 7)) ) )  (*  (sin  (cos  (list-ref X 8)) )  (gp-log  (-  (list-ref X 6) (list-ref X 5)) ) ) )  (-  (-  (gp-log  (list-ref X 6))  (-  (sin  (list-ref X 5))  (gp-log  (list-ref X 0)) ) )  (sin  (sin  (+  (-  (-  (-  (+  (+  (cos  (list-ref X 6))  (fdim  (list-ref X 4) (list-ref X 0)) )  (abs  (gp-log  (list-ref X 3)) ) )  (*  (gp-log  (+  (list-ref X 4) (list-ref X 8)) )  (cos  (sin  (fdim  (-  (list-ref X 0) (list-ref X 9))  (abs  (list-ref X 8)) ) ) ) ) )  (-  (-  (cos  (fdim  (list-ref X 2) (list-ref X 9)) )  (-  (*  (gp-log  (*  (*  (abs  (-  (abs  (list-ref X 8))  (+  (list-ref X 5) (list-ref X 5)) ) )  (sin  (list-ref X 1)) )  (cos  (sin  (list-ref X 1)) ) ) )  (-  (+  (cos  (cos  (list-ref X 0)) )  (+  (fdim  (list-ref X 2) (list-ref X 9))  (abs  (list-ref X 1)) ) )  (cos  (cos  (gp-log  (abs  (-  (list-ref X 5) (list-ref X 7)) ) ) ) ) ) )  (-  (sin  (list-ref X 5))  (gp-log  (list-ref X 0)) ) ) )  (abs  (+  (list-ref X 2) (list-ref X 9)) ) ) )  (abs  (-  (list-ref X 6) (gp-log  (list-ref X 7)) ) ) )  (list-ref X 9)) ) ) ) )  (+  (fdim  (-  (sin  (-  (-  (+  (cos  (+  (cos  (-  (gp-log  (-  (list-ref X 0) (abs  (list-ref X 8)) ) )  (-  (-  (list-ref X 1) (list-ref X 8))  (sin  (list-ref X 7)) ) ) )  (sin  (cos  (cos  (sin  (list-ref X 5)) ) ) ) ) )  (cos  (*  (list-ref X 7) (list-ref X 7)) ) )  (*  (sin  (cos  (list-ref X 8)) )  (gp-log  (-  (list-ref X 6) (list-ref X 5)) ) ) )  (-  (-  (cos  (fdim  (list-ref X 2) (list-ref X 9)) )  (-  (cos  (fdim  (list-ref X 8) (list-ref X 6)) )  (gp-log  (list-ref X 0)) ) )  (*  -0.960798442 (list-ref X 5)) ) ) )  (cos  (+  (+  (cos  (list-ref X 6))  (list-ref X 9))  (abs  (gp-log  (list-ref X 3)) ) ) ) )  (+  (gp-log  (list-ref X 1))  (cos  (gp-log  (list-ref X 9)) ) ) )  (fdim  (+  (+  (cos  (list-ref X 6))  (*  (list-ref X 7) (list-ref X 7)) )  (abs  (gp-log  (list-ref X 3)) ) )  (gp-log  (-  (list-ref X 6) (list-ref X 5)) ) ) ) ) ))
;;Time(Symbolic Regression (End-to-End)) = 21.704914346 sec
