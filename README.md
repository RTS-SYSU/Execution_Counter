# Execution_Counter

A simple execution time counter use to eval [llvmta](https://github.com/RTS-SYSU/llvmta)

# Usage

Please change the `CoreInfo.json` to change the test set.

## Basic Usage

The `CoreInfo.json` is similar to what we used in [llvmta](https://github.com/RTS-SYSU/llvmta), you can check that project for further info, besides, we provide a simple example in [CoreInfo.json](./CoreInfo.json).

## Advanced Usage

If you want to test your own test function, please follow below:

1. Write your own test function in `test` directory, and make sure it returns a `void` and receives a `void*` as its only parameter.

2. Change the `CoreInfo.json` to use your own test function.

3. Compile and run as below.

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
LD_LIBRARY_PATH=. ./driver <test iteration> <Core info.json> <output.json>
```

## Evaluation

We also provide a simple evaluation script in `eval.py`, you can use it to evaluate the output json file.

Please be noted that you have to provide a json file looks like `wcet.json`, we provide an [example](./wcet.json), which is the result of [llvmta](https://github.com/RTS-SYSU/llvmta), and all arguments can be found by `python eval.py -h`.

An example is shown below:

```bash
./eval.py --json output.json --wcet wcet.json --calc mean median max min std --plot
```

This will show the results on the screen and plot the result in certain format.