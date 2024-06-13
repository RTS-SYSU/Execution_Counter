# Execution_Counter

A simple execution time counter use to eval [llvmta](https://github.com/RTS-SYSU/llvmta)

# Usage

Please change the `CoreInfo.json` to change the test set.

## Basic Usage

The `CoreInfo.json` is similar to what we used in [llvmta](https://github.com/RTS-SYSU/llvmta), you can check that project for further info, besides, we provide a simple example in [CoreInfo.json](./CoreInfo.json).

Here is a example

```json
[
    {
        "core": 0,
        "tasks": [
            {
                "function": "my_loop",
                "lib": "libtestfunc.so"
            }
        ]
    },
    {
        "core": 1,
        "tasks": [
            {
                "function": "my_loop2",
                "lib": "libtestfunc.so"
            }
        ]
    },
    {
        "core": 2,
        "tasks": [
            {
                "function": "my_loop3",
                "lib": "libtestfunc.so"
            }
        ]
    },
    {
        "core": 3,
        "tasks": [
            {
                "function": "my_loop4",
                "lib": "libtestfunc.so"
            }
        ]
    }
]
```

This will test the function `my_loop` in `libtestfunc.so` on core 0, `my_loop2` in `libtestfunc.so` on core 1, `my_loop3` in `libtestfunc.so` on core 2, and `my_loop4` in `libtestfunc.so` on core 3.

Please make sure the `libtestfunc.so` is in the same directory with the `driver`, and the test function is marked with `extern "C"` if you are using C++.

Note that all function prototype should be `void func(void *)`, and currently the argument is not used, it will always be `NULL`.

## Build

Just simply run `make` in the root directory of this project.

If you need to change the test function flags, use

```bash
make TEST_FLAGS=<your flags> TEST_CC=<your compiler>
```

Note that by default, we use gcc to compile the framework and clang to compile the test function.

## Run

Please run it by 

```bash
LD_LIBRARY_PATH=. ./driver <test iteration> <Coreinfo.json> <output.json> [-e <event_id>]
```

if you want to use the perf event counter, please provide the event id by `-e <event_id>`, you can use `./driver -h` to show, here is an example:

```text
Usage: ./driver <repeats> <input_json> <output_json> [-e perf_event_id]
        Current support perf_event:
                0: cache-misses
                1: cache-references
                2: L1-icache-load-misses
                3: L1-dcache-load-misses
                4: L1-icache-loads
                5: L1-dcache-loads
                6: L1-dcache-store-misses
                7: bus-cycles
                8: L1-icache-prefetch-misses
                9: L1-dcache-prefetch-misses
                10: L1-icache-prefetches
                11: L1-dcache-prefetches
```

By default, if you do not provide the event id, it will just count the cycles by `rdtsc` if you are using x86 or `pmccntr` if you are using ARM.

Specially, for ARM user, please make sure you have the permission to access the `pmccntr`. As by default, ARM does not allow user to access them in user space, you may refer [armv8_pmu_cycle_counter_el0](https://github.com/jerinjacobk/armv8_pmu_cycle_counter_el0) to install the kernel module to enable it.

Also note that not all OS support the perf event counter, you may need to check the support of your OS.

## Evaluation

We also provide a simple evaluation script in `eval.py`, you can use it to evaluate the output json file.

Please be noted that you have to provide a json file looks like `wcet.json`, we provide an [example](./wcet.json), which is the result of [llvmta](https://github.com/RTS-SYSU/llvmta), and all arguments can be found by `python eval.py -h`.

An example is shown below:

```bash
./eval.py --json output.json --wcet wcet.json --calc mean median max min std --plot
```

This will show the results on the screen and plot the result in certain format.