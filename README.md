# Parallel Sort by Regular Sampling

## Structure
``` text
.
├── CMakeLists.txt
├── app
│   └── main.cpp
├── include
│   ├── psrs.hpp
│   └── utils.hpp
├── src
│   └── utils.cpp
├── tests
│   └── main.cpp
├── experiments
│   ├── correctness
│   │   # CSV files with data from experiments
│   │
│   ├── performance
│   │   # CSV files with data from experiments
│   │  
│   ├── phases
│   │   # CSV files with data from experiments
│   │
│   └── sampling
│       # CSV files with data from experiments
└─── experiments.ipynb # Jupyter notebook with experiments
```

## Building

Build by making a build directory (i.e. `build/`), run `cmake` in that dir, and then use `make` to build the desired target.

Example:

``` bash
> mkdir build && cd build
> cmake .. -DENABLE_PHASE_TIMING=OFF -DENABLE_DEBUG=OFF -DENABLE_RANDOM_SAMPLING=OFF -DENABLE_NORMAL=OFF -DENABLE_UNIFORM=OFF
> make
> ./main {p} {n} {s} {useUniprocessor (0 or 1)}
```

## Running experiments

To run experiments, open the `experiments.ipynb` notebook and run the cells.