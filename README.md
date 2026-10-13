# CS598APE HW2 — Symbolic Regression Performance Engineering

This repository contains an optimized C++ implementation of the symbolic regression and classification benchmark provided for UIUC CS598APE Homework 2. The project evaluates candidate mathematical expressions by genetic programming. This submission focuses on runtime optimization **without changing the benchmark configurations or the generated results** on the three evaluated datasets.

The original project documentation, including image reconstruction, Racket output, and Docker instructions, is preserved below.

## Build and requirements

The optimized CPU benchmark has been developed and tested on a university Ubuntu Linux VM with **4 virtual CPUs**, using **GCC/G++ 15.2**, GNU Make, and OpenMP. No GPU is needed.

Requirements for the three tabular benchmarks:

- A C++17-capable `g++` with OpenMP support (`-fopenmp`)
- GNU `make`
- A Linux/Unix-like terminal (Bash for the validation script below)

Racket is **optional for the three tabular benchmark runs**; it is used to execute/render the generated Racket programs and to reconstruct images.

```bash
# From the repository root
make clean
make -j

# The default Makefile builds with:
# -std=c++17 -O3 -fopenmp
```

The executable is `./genetic_benchmark`. To run each evaluated dataset using the same four-thread configuration as the final experiments:

```bash
OMP_NUM_THREADS=4 ./genetic_benchmark diabetes
OMP_NUM_THREADS=4 ./genetic_benchmark cancer
OMP_NUM_THREADS=4 ./genetic_benchmark housing
```

The benchmark prints the synthesized programs in Racket-compatible form and ends with a `;;Time(...) = ... sec` line reporting end-to-end execution time. Redirect stdout to a file if you want to save the output.

## Implemented optimizations

The optimized source contains three code-level changes, in addition to compiler optimization:

1. **Constant-time register stack operations** (`src/reg_stack.h`): replaces full-array scans during `push`/`pop` with direct indexed access to the active stack position.
2. **OpenMP parallel program evaluation** (`src/program.cpp`): parallelizes evaluation across independently processed candidate programs in `execute_kernel`, with static scheduling and four threads during benchmarks.
3. **Node-storage reuse during program assignment** (`src/program.cpp`): avoids unnecessary deallocation and allocation when assigning a program to storage that can be reused, reducing candidate-copy overhead, particularly for the cancer workload.

The default `Makefile` uses `-O3 -fopenmp`. A separate correctness fix initializes `y_mu` in `weightedPearson` (`include/fitness.h`), preventing use of an uninitialized accumulator. This fix is not counted as a performance optimization.

## Benchmark results

All three evaluated workloads were run on the same university VM. The baseline refers to the original, unoptimized instructor implementation built without an explicit optimization flag; the final version is built with `-O3 -fopenmp` and run with `OMP_NUM_THREADS=4`.

| Dataset | Original baseline (s) | Optimized mean (s) | Overall speedup |
|---|---:|---:|---:|
| Diabetes | 21.59 | 0.443 | 48.7x |
| Cancer | 135.01 | 9.267 | 14.6x |
| Housing | 249.57 | 6.207 | 40.2x |

The optimized means are the arithmetic means of **three wall-clock runs**, captured using `/usr/bin/time -p`, after the third code optimization. The baseline entries are **single reference measurements**, not three-run averages. Consequently, speedups should be treated as representative measurements rather than confidence intervals. Execution times depend on machine load and CPU configuration.

### Final optimization: paired A/B measurements

The following runs isolate the effect of the node-storage reuse change. Both versions include `-O3`, the stack optimization, and OpenMP with four threads.

| Dataset | Before reuse (s; 3 runs) | After reuse (s; 3 runs) | Before mean (s) | After mean (s) |
|---|---|---|---:|---:|
| Diabetes | 0.44, 0.46, 0.46 | 0.44, 0.45, 0.44 | 0.453 | 0.443 |
| Cancer | 10.95, 11.02, 10.91 | 9.19, 9.12, 9.49 | 10.960 | 9.267 |
| Housing | 6.19, 6.23, 6.30 | 6.17, 6.17, 6.28 | 6.240 | 6.207 |

The cancer runtime improved by approximately **15.5%** in this paired comparison. The small differences for diabetes and housing should not be interpreted as statistically significant improvements.

### OpenMP scaling (housing)

Housing was also tested with one, two, and four OpenMP threads, three times each, before node-storage reuse:

| Threads | Run times (s) | Mean (s) | Relative speedup vs. 1 thread |
|---:|---|---:|---:|
| 1 | 20.69, 20.80, 20.74 | 20.743 | 1.00x |
| 2 | 11.23, 11.15, 11.18 | 11.187 | 1.85x |
| 4 | 6.62, 6.44, 6.54 | 6.533 | 3.18x |

## Correctness verification

For **diabetes**, **cancer**, and **housing**, the final optimized output was compared against the saved original baseline output. In each case, comparison excluded only the elapsed-time comment (`;;Time(...)`) and returned **MATCH**. The model output itself was not intentionally changed.

To repeat the optimized runs and save outputs locally:

```bash
mkdir -p results/reproduction
for dataset in diabetes cancer housing; do
    OMP_NUM_THREADS=4 /usr/bin/time -p \
        ./genetic_benchmark "$dataset" \
        > "results/reproduction/${dataset}.txt"
done
```

If you have generated or retained the original baseline outputs at `results/baseline/<dataset>.txt`, verify their content against the optimized output using Bash:

```bash
for dataset in diabetes cancer housing; do
    if diff -q \
        <(sed '/^;;Time(/d' "results/baseline/${dataset}.txt") \
        <(sed '/^;;Time(/d' "results/reproduction/${dataset}.txt"); then
        echo "$dataset: MATCH"
    else
        echo "$dataset: DIFFERENT"
    fi
done
```

**Note:** `results/baseline/` contains locally generated reference outputs from experimentation; these reference files are not assumed to be present in a fresh clone. To obtain reference results independently, build and run the unmodified instructor version with its original compiler settings on the same machine before comparing outputs. The exact outputs and runtimes can depend on toolchain and runtime environment.

The `weightedPearson` initialization fix was successfully compiled and the three benchmark results were rechecked, but **a separate Pearson-specific functional test has not yet been reported**. Similarly, the two image-reconstruction workloads supplied by the original project were not part of the reported three-dataset validation.

## Profiling and methodology

The optimization process used compiler comparisons (`-O2`, `-O3`, exploratory profile-guided optimization), GNU `gprof`, output-preserving A/B tests, and OpenMP thread-scaling experiments. The final default build is `-O3 -fopenmp`; it **does not depend on profile-guided optimization files**.

The university VM restricted Linux `perf` sampling (`perf_event_paranoid=4`), so GNU `gprof` was used for profiling. Instrumentation and OpenMP can affect sampling quality; the profiling percentages should not be treated as precise wall-clock breakdowns. All reported timing comparisons use the uninstrumented benchmark binary.

For the final benchmark, run multiple times under a consistent environment. The following command collects three runs for each dataset:

```bash
mkdir -p results/repeated_runs
for dataset in diabetes cancer housing; do
    for run in 1 2 3; do
        OMP_NUM_THREADS=4 /usr/bin/time -p \
            ./genetic_benchmark "$dataset" \
            > "results/repeated_runs/${dataset}_${run}.txt" \
            2> "results/repeated_runs/${dataset}_${run}_time.txt"
    done
done
```

## Source layout

| Path | Purpose |
|---|---|
| `benchmark/genetic_benchmark.cpp` | End-to-end benchmark driver and datasets |
| `src/program.cpp` | Expression evaluation, OpenMP parallelism, program copying |
| `src/reg_stack.h` | Optimized expression evaluation stack |
| `include/fitness.h` | Fitness functions, including Pearson initialization fix |
| `src/philox_rng.cpp`, `include/philox_rng.h` | Random-number generator |
| `Makefile` | Reproducible default optimized build |
| `benchmark/` | Benchmark assets, including image inputs |

The original repository also provides **silk-wave** and **square-wave** reconstruction demonstrations. Those tasks use Racket for rendering and are documented in the original section below.

---

## Original Project Documentation

This repository contains code for homework 2 of 598APE.

In particular, this repository is an implementation of Symbolic Regression and the various contexts it can be used in.

To compile the program run:
```bash
make -j
```

To clean existing build artifacts run:
```bash
make clean
```

This program assumes the following are installed on your machine:
* A working C++ compiler (g++ is assumed in the Makefile)
* make
* racket (to plot images)

Running make builds a sample benchmarking binary(`genetic_benchmark`) which you will use for perf engineering. However, note that the symbolic regression routines exposed in `genetic.h` are general, and can be used as a sub-routine in any data science pipeline (pending a Cython wrapper to Python/Torch).

Once compiled, one can run the benchmark code on the diabetes dataset as follows. The output is a valid racket program, which provides the 2 best programs as callable functions - symbolic regression is used for synthesizing these functions. 

```bash
./genetic_benchmark diabetes
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
;;Time(Symbolic Regression (End-to-End)) = 11.512300000 sec

```

`(list-ref X i)` corresponds to the `i`th feature of your dataset (it is Racket syntax to access `X[i]`)

While currently hardcoded to only handle the 3 CSV datasets and 2 image reconstruction tasks, `genetic_benchmark.cpp` in the `benchmarks` folder exposes training code for end to end symbolic regression/binary classification, starting from converting the input data to a columnar format, before dispatching it to `genetic::symFit`. 

The behaviour of the training run can be tuned through the `param` struct, which controls paramenters like the population size, functions being considered, subtree depth and so on.  

```bash
./genetic_benchmark diabetes
```

There are also 2 image reconstruction tasks. The goal of image re-construction is to infer the underlying semantic meaning present in an image. We do this by representing every image as the average of `K` mathematical equations, and output the equations as a valid racket program (which you can also use to draw the final image). All input images have 256x256 pixels, and each pixel stores a r,g,b tuple corresponding to red, green and blue pixel values. For example, on the silk-wave input:

![Silk wave](./benchmark/silk-waves.png)

We now run symbolic regression based reconstruction on the silk-wave picture. 

```bash
./genetic_benchmark silk-wave > silk-ref.rkt
racket silk-ref.rkt silk-waves-reconstructed.png
```

This produces 2 artifacts. [silk-ref.rkt](./benchmark/silk-ref.rkt) contains the symbolic definition of the image. The reconstructed image is 

![Reconstructed Silk wave](./benchmark/silk-waves-reconstructed.png)

Similarly, for the harder square wave benchmark, we start from 

![Square wave](./benchmark/squared-waves.png)

and are able to reconstruct 

![Reconstructed square wave](./benchmark/square-wave-reconstructed.png)

with the source code for the reconstructed wave in [square-wave-ref.rkt](./benchmark/square-wave-ref.rkt)

The time taken for the end to end run can be seen in the last comment of the generated rkt file. Similar to HW1, your goal is to reduce this runtime as much as possible, while maintining/increasing complexity. For the image code, it is sufficient to just optimize image generation, however, you are also free to optimize image drawing too (which is currently implemnented as a DSL in Racket)

## Input Datasets and Images
This project contains 3 input datasets and 2 images for you to optimize. All 3 are standard scikit-learn datasets. The images are generated bitmaps from `benchmark/generate-easy-image.rkt`.

### Diabetes

```bash
./genetic_benchmark diabetes
```

### Cancer

```bash
./genetic_benchmark cancer
```

### California Housing Prices

```bash
./genetic_benchmark housing
```

### Silk waves

```bash
./genetic_benchmark silk-wave > silk.rkt
racket silk.rkt silk.png
```

### Square waves

```bash
./genetic_benchmark square-wave > square.rkt
racket square.rkt square.png
```

## Code Overview

There are multiple core functions

`cpp_evolve`

`cpp_evolve` is the central evolution driver. It takes a population of candidate programs and produces a new generation by first selecting parents through tournament selection and then applying various mutation strategies (crossover, subtree mutation, hoist mutation, and point mutation) or direct reproduction. After mutation, it updates the fitness of the new generation.

`build_program`

`build_program` constructs an initial candidate program (represented as a syntax tree) using a randomized process. It chooses function nodes and terminal nodes based on specified initialization methods (grow, full, or half-and-half) and ensures that the generated program adheres to a maximum depth constraint.

`tournament_kernel` 

`tournament_kernel` implements the tournament selection process. It randomly samples subsets of the population, evaluates their fitness (including a parsimony penalty for program length), and selects the best candidates to serve as parents for the next generation.

`execute_kernel`

Given a bunch of programs(symbolic formulae), `execute_kernel` evaluates those formulae on the given input dataset and outputs predicted values. We evaluate all programs on all dataset rows. 

`compute_metric`

As the name suggests, we compute the loss/accuracy values according to the metric specified in `params`. There are 5 different metrics supported - logLoss (for classification), mean square error and root mean square error(for regression) and correlation coefficients for transformation (both Karl Pearson and Spearman's rank correlation are implemented). 

## Docker

For ease of use and installation, we provide a docker image capable of running and building code here. The source docker file is in /docker (which is essentially a list of commands to build an OS state from scratch). It contains the dependent compilers, and some other nice things.

Run the docker build for the first time to setup all the environments you need
```bash
./dockerbuild.sh
```

If you now run `./dockerrun.sh` you will enter an interactive bash session with all the packages from docker installed (that script by default uses `vim/598ape`, feel free to replace it with whatever location you like if you built from scratch). The current directory (aka this folder) is mounted within `/host`. Any files you create on your personal machine will be available there, and anything you make in the container in that folder will be available on your personal machine.
