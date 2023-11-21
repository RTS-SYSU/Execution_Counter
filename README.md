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

if you need to change the number of cores or change the tasks, please follow the steps below:

1. change the `#define CORE_NUM 4` to the number of cores you want to use

2. Add your task function, for now, all task functions should be like `void func(void* args)`

3. Add your task function to the `func` array in `main` function

4. Add your task function name to the `funcname` array in `main` function

5. Change the `args` array in `main` function, the format must follow: 

```c
uint64_t args[TOTAL_CORE][/*Fill this by your self*/] = {
    {
        (uint64_t)1,                // how man functions in this core
        (uint64_t)funcname[0],      // function name
        (uint64_t)func[0],          // function address, a function pointer
        (uint64_t)NULL,             // function args, if no args, set to NULL
    },
    /* Add other cores */
};
```

## Build

Just simply run `make` in the root directory of this project