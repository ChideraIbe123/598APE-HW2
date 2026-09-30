#lang racket

(require racket/draw)
(define width 256)
(define height 256)

;; Generate png img
(define (generate filename channels)
  (define pixels (make-bytes (* 4 width height)))
  (for* ([row (in-range height)]
         [col (in-range width)])
    (define x (/ (+ col 0.5) width))
    (define y (/ (+ row 0.5) height))
    (define u (* pi (- (* 2 x) 1)))
    (define v (* pi (- (* 2 y) 1)))
    (define rgb (channels u v))
    (define offset (* 4 (+ (* row width) col)))
    (bytes-set! pixels offset 255)
    (for ([value rgb]
          [channel (in-naturals 1)])
      (bytes-set! pixels (+ offset channel) (exact-round (* 255.0 value)))))
  (define bitmap (make-bitmap width height))
  (send bitmap set-argb-pixels 0 0 width height pixels)
  (unless (send bitmap save-file filename 'png)
    (error 'generate-easy-image "Could not save ~a" filename)))

(generate "benchmark/interference.png"
          (lambda (u v)
            (define intensity (abs (sin (* u v))))
            (list intensity intensity intensity)))
(generate "benchmark/interference-color.png"
          (lambda (u v) (list (abs (sin (* u v))) (abs (sin u)) (abs (sin v)))))

(generate "benchmark/triangle-facets.png"
          (lambda (u v)
            (define intensity (/ (+ (/ (abs u) pi) (/ (abs v) pi)) 2.0))
            (list intensity intensity intensity)))

(generate "benchmark/silk-waves.png"
          (lambda (u v) (list (abs (sin (* u v))) (abs (sin u)) (abs (sin (* v v))))))

;; Crossed waves: three short expressions using the existing scalar nodes.
(generate "benchmark/squared-waves.png"
          (lambda (u v) (list (abs (sin (* u u))) (abs (sin (* u v))) (abs (sin (* v v))))))

;; A harder candidate: sums and differences of squared coordinates
;; produce circular and hyperbolic wave contours.
(generate
 "benchmark/radial-waves.png"
 (lambda (u v)
   (list (abs (sin (+ (* u u) (* v v)))) (abs (sin (- (* u u) (* v v)))) (abs (sin (* u v))))))
