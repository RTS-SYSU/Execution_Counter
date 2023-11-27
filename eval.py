#!/usr/bin/env python3

import argparse
import sys
import json
import numpy as np

import matplotlib.pyplot as plt

def data_cleaning(data, std_range = 5):
    mean = np.mean(data)
    std = np.std(data)
    data = [x for x in data if x < mean + std_range * std and x > mean - std_range * std]
    return data


def plot_result(data, wcet, title: str, output: str):
    data = data_cleaning(data)
    x = [i for i in range(len(data))]
    data = [data[i] / wcet * 100 for i in range(len(data))]
    
    plt.title(title)
    plt.xlabel('Execution')
    plt.ylabel('Ticks/WCET (%)')
    plt.scatter(x, data, s=10, c='b', marker="s", label='ticks')
    
    plt.savefig(output)
    
    plt.cla()
    plt.close()


def read_json(file_name):
    with open(file_name, 'r') as f:
        data = json.load(f) 
    return data


def process_json(data) -> dict:
    ret = dict()
    for core_info in data:
        core = core_info['core']
        ret[core] = list()
        for res in core_info['results']:
            tmp = list()
            for task in res['tasks']:
                tmp.append(tuple([task['function'], task['ticks']]))
            ret[core].append(tuple(tmp))
    return ret


def calc(data, method, wcet, plot):
    for core in data:
        print(f'Core: {core}')
        res_core = list()
        names = list()
        fill_names = False
        
        for each_result in data[core]:
            # each_result : tuple([])
            # print(f'each_result: {len(each_result)}')
            # len(each_result): How many tasks were executed
            tmp_res = list()
            # print(f'shape: {tuple([len(each_result[0]), len(each_result)])}')
            for each_task_index in range(len(each_result)):
                # print(each_result[each_task_index])
                
                if fill_names is False:
                    # print(f'task : {each_result[each_task_index][0]}')
                    names.append(each_result[each_task_index][0])
                tmp_res.append(each_result[each_task_index][1])
            tmp_res = np.array(tmp_res)
            res_core.append(tmp_res)
            fill_names = True
        
        tasks = len(res_core[0])
        res_core = np.array(res_core)
        if plot:
            for i in range(tasks):
                plot_result([res_core[x][i] for x in range(len(res_core))], wcet[names[i]],f'Core: {core} Task: {names[i]}', f'core_{core}_task_{names[i]}.png')
        res_sum = np.sum(res_core, axis=0)
        for i in range(len(res_sum)):
            print(f'sum: {names[i]}: {res_sum[i]}')
        if ('mean' in method):
            print('mean:')
            res = np.mean(res_core, axis=0)
            for i in range(len(res)):
                print(f'{names[i]}: {res[i]}')
        if ('median' in method):
            print('median:')
            res = np.median(res_core, axis=0)
            for i in range(len(res)):
                print(f'{names[i]}: {res[i]}')
        if ('max' in method):
            print('max:')
            res = np.max(res_core, axis=0)
            for i in range(len(res)):
                print(f'{names[i]}: {res[i]}')
        if ('min' in method):
            print('min:')
            res = np.min(res_core, axis=0)
            for i in range(len(res)):
                print(f'{names[i]}: {res[i]}')
        if ('std' in method):
            print('std:')
            res = np.std(res_core, axis=0)
            for i in range(len(res)):
                print(f'{names[i]}: {res[i]}')
        if ('mean' in method and 'std' in method):
            # mead+=5std
            print('mean + 5std:')
            res = np.mean(res_core, axis=0) + 5 * np.std(res_core, axis=0)
            for i in range(len(res)):
                print(f'{names[i]}: {res[i]}')
        print('-------------------------------------------')
    return 0


def main(args) -> int:
    data = read_json(args.json)
    res = process_json(data)
    _wcets = read_json(args.wcet)
    wcet = dict()
    
    for fun in _wcets:
        wcet[fun['function']] = fun['WCET']
    
    calc(res, args.calc, wcet, args.plot)
    
    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser('Evaluatioin')
    
    parser.add_argument("--json", type=str, default='output.json', help='Json file to be evaluated')
    parser.add_argument("--wcet", type=str, default='wcet.json', help='Json file of wcet')
    parser.add_argument("--calc", choices=["mean", "median", "max", "min", "std"], default=["mean", "median", "max", "min", "std"], help="Calculation method", nargs='+')
    parser.add_argument("--plot", action='store_true', help="Plot the result")
    
    args = parser.parse_args()    
    sys.exit(main(args))