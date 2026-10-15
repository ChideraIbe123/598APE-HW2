# 598APE-HW2

I sped up the symbolic regression benchmark by 105x to 338x. The output is exactly the same as the original code, other than the timer line at the end.

## Tools

Everything you need is already in the Docker image (clang 19, libomp, make):

```bash
./dockerbuild.sh
./dockerrun.sh
cd /host
```

## Build and run

```bash
make -j
./genetic_benchmark diabetes
```

The time is the last line of the output. The other benchmarks are `housing`, `cancer`, `silk-wave` and `square-wave`.

## Checking the output

`golden/` has the output from the original code. To check that a run matches it:

```bash
./genetic_benchmark cancer > out.rkt
diff <(head -n -1 out.rkt) <(head -n -1 golden/cancer.rkt) && echo same
```

## Running the baseline

The original code is commit `f19cc63`. It's slow (around 40 minutes for all 5).

```bash
git worktree add ../baseline f19cc63
cd ../baseline && make -j
./genetic_benchmark diabetes
```

## Optimizations

Each optimization is its own commit. To test one, build that commit and the one before it the same way as the baseline, and compare the times. All times are in seconds, on a 4-core machine.

| # | commit | change | diabetes | housing | cancer | silk | square |
|---|---|---|---:|---:|---:|---:|---:|
| 0 | `f19cc63` | original code | 21.70 | 250.06 | 134.02 | 532.41 | 1326.39 |
| 1 | `42a24b8` | `clang++ -O3 -flto` | 2.11 | 29.32 | 19.55 | 43.23 | 123.49 |
| 2 | `59addf7` | faster stack push/pop | 2.18 | 23.74 | 18.45 | 38.04 | 106.76 |
| 3 | `0c5cb92` | top-2 selection instead of insertion sort | 1.92 | 24.35 | 13.57 | 37.42 | 105.66 |
| 4 | `c34d052` | one-pass logLoss/MSE | 2.08 | 24.93 | 13.33 | 31.18 | 95.96 |
| 5 | `2fd7579` | evaluate 64 rows per tree walk | 0.55 | 11.39 | 7.37 | 12.39 | 33.88 |
| 6 | `7c0f936` | fitness cache for duplicate programs | 0.442 | 7.547 | 2.888 | 4.63 | 15.45 |
| 7 | `1cfba00` | faster cache keys | 0.402 | 7.538 | 2.598 | 4.64 | 15.74 |
| 8 | `9cd73d5` | subtree cache | 0.142 | 3.500 | 2.130 | 3.53 | 11.62 |
| 9 | `e213a96` | OpenMP (`-fopenmp`) | 0.127 | 1.593 | 1.271 | 1.576 | 5.185 |
