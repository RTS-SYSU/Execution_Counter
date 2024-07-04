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

This will test the function `my_loop` in `libtestfunc.so` on core 1, `my_loop2` in `libtestfunc.so` on core 2, `my_loop3` in `libtestfunc.so` on core 3, and `my_loop4` in `libtestfunc.so` on core 4, while core 0 in reserved for the system.

Please make sure the `libtestfunc.so` is in the same directory with the `driver`, and the test function is marked with `extern "C"` if you are using C++.

Note that all function prototype should be like `int func(void)`, this is because we want to make the 

## Build

Just simply run `make` in the root directory of this project.

Note that the default compiler is `gcc` for both the test function and the driver, and the default test flags is `-O0`, change them if you need.

If you need to change the test function flags, use

```bash
make TEST_FLAGS=<your flags> TEST_CC=<your compiler>
```

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
                12: instructions
```

By default, if you do not provide the event id, it will just count the cycles by `rdtsc` if you are using x86 or `pmccntr` if you are using ARM.

Specially, for ARM user, please make sure you have the permission to access the `pmccntr`. As by default, ARM does not allow user to access them in user space, you may refer [armv8_pmu_cycle_counter_el0](https://github.com/jerinjacobk/armv8_pmu_cycle_counter_el0) to install the kernel module to enable it.

Also note that not all OS support the perf event counter, you may need to check the support of your OS.

Additionally, the instruction count may not be accurate, according to the [perf_event_open](https://man7.org/linux/man-pages/man2/perf_event_open.2.html), as the hardware interrupt may affect the result. One way to use it is to run the instruction count multiple times and take the most common result as the final result.

## Evaluation

> Note: for now, we do not provide the evaluation script in dev branch, you may need to write your own script to evaluate the result.

The default output is a json file, which is a list of core results, each core result contains a list of task results, each task result contains the function name and the ticks(or the perf event you choose) it takes.

```json
[
	{
		"core": 0,
		"results": [
			{
				"id": 0,
				"tasks": [
					{
						"function": "my_loop",
						"ticks": 798
					}
				]
			}
		]
	},
	{
		"core": 1,
		"results": [
			{
				"id": 0,
				"tasks": [
					{
						"function": "my_loop2",
						"ticks": 4484
					}
				]
			}
		]
	},
	{
		"core": 2,
		"results": [
			{
				"id": 0,
				"tasks": [
					{
						"function": "my_loop3",
						"ticks": 4332
					}
				]
			}
		]
	},
	{
		"core": 3,
		"results": [
			{
				"id": 0,
				"tasks": [
					{
						"function": "my_loop4",
						"ticks": 274322
					}
				]
			}
		]
	}
]
```

