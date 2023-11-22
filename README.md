# Execution_Counter

A simple execution time counter use to eval [llvmta](https://github.com/RTS-SYSU/llvmta)

# Usage

Please change the `driver.c` to enable test.

## Basic Usage

In the `driver.c`, We provide a simple example, which is a 4-core parallel execution of different tasks

```text
function: 0x55c0be2d7199, function name: my_loop, ticks: 808
function: 0x55c0be2d7237, function name: my_loop3, ticks: 5632
function: 0x55c0be2d71dc, function name: my_loop2, ticks: 4720
function: 0x55c0be2d7290, function name: my_loop4, ticks: 592628
```

## Modification

We make it more easy to use, for now, all you need to do is:

1. Add your function in `test/funcimpl.c` and `test/funcdef.h`

2. Modified `driver.c`, change the macro `CORE` to your system core, and then call `add_function` to do the task set.

More specifically, first call `create_test_args(CORE)` to get an array of different core task info, then call `add_function(&arg[core_number], func_name, func_ptr, func_args)` to add the function to specific core

3. call `start_test(CORE, arg)` to start

4. call `free_test_args(arg)` to free the args we create in step 2.

We provide a simple example in `driver.c`, change it by your need.

## Build

Just simply run `make` in the root directory of this project

## Run

Please run it by 

```bash
LD_LIBRARY_PATH=. ./driver
```
