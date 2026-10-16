#lang racket

;;===== Symbolic Classification Benchmark =====

(provide best-program-1 best-program-2)
;;Loading dataset...
;;Dataset dimensions: 569 samples x 30 features
;;Training symbolic classifier with 16384 population size and 16 generations 
;;Best program 1 details:
;;- Length: 20 nodes
;;- Depth: 5
;;- Raw fitness: 0.15991
;;- Test accuracy: 0.946903
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
  (-  (+  (cos  (*  (list-ref X 21) (list-ref X 7)) )  (-  (*  (fdim  (list-ref X 24) (list-ref X 27))  (-  (list-ref X 22) (list-ref X 13)) )  (*  (list-ref X 21) (list-ref X 7)) ) )  (cos  (gp-sqrt  (list-ref X 22)) ) ) ))
;;Best program 2 details:
;;- Length: 20 nodes
;;- Depth: 5
;;- Raw fitness: 0.15991
;;- Test accuracy: 0.946903
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
  (+  (cos  (*  (list-ref X 21) (list-ref X 7)) )  (-  (-  (*  (fdim  (list-ref X 24) (list-ref X 27))  (-  (list-ref X 22) (list-ref X 13)) )  (*  (list-ref X 21) (list-ref X 7)) )  (cos  (gp-sqrt  (list-ref X 22)) ) ) ) ))
;;Time(Symbolic Classification (End-to-End)) = 127.713039427 sec
