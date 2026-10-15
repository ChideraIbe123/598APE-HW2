#lang racket

;;===== Symbolic Regression Benchmark =====

(provide best-program-1 best-program-2)
;;Loading dataset...
;;Dataset dimensions: 20640 samples x 8 features
;;Training symbolic regressor with 512 population size and 16 generations
;;Best program 1 details:
;;- Length: 28 nodes
;;- Depth: 7
;;- Raw fitness: 0.659991
;;- Test MSE: 0.64586
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
  (*  (cos  (sin  (gp-log  (+  (list-ref X 0) (list-ref X 0)) ) ) )  (-  (gp-log  (fdim  (*  (list-ref X 2) (list-ref X 0))  (fdim  (list-ref X 0) (list-ref X 6)) ) )  (-  (cos  (abs  (list-ref X 2)) )  (sin  (cos  (fdim  (*  (list-ref X 5) 0.798811913)  (gp-log  (list-ref X 0)) ) ) ) ) ) ) ))
;;Best program 2 details:
;;- Length: 23 nodes
;;- Depth: 5
;;- Raw fitness: 0.754433
;;- Test MSE: 0.728984
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
  (*  (cos  (sin  (gp-log  (+  (list-ref X 0) (list-ref X 0)) ) ) )  (-  (gp-log  (fdim  (*  (list-ref X 2) (list-ref X 0))  (fdim  (list-ref X 0) (list-ref X 6)) ) )  (-  (cos  (abs  (list-ref X 2)) )  (sin  (cos  (list-ref X 3)) ) ) ) ) ))
;;Time(Symbolic Regression (End-to-End)) = 250.063715689 sec
