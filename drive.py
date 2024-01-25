#!/usr/bin/env python3

import re
import subprocess
import sys
import json
import os

def init_json(input_json: str):
    with open(input_json, 'r') as f:
        data = json.load(f)
    core = len(data)
    
    ret = list()
    
    for i in range(core):
        tmp = dict()
        tmp['core'] = i
        tmp['results'] = list()
        ret.append(tmp)
    
    return ret

def process_json(output_json: str, data: list, id: int):
    with open(output_json, 'r') as f:
        result = json.load(f)
    
    for i in range(len(result)):
        tmp = dict()
        tmp['id'] = id
        tmp['tasks'] = result[i]['results'][0]['tasks']
        data[i]['results'].append(tmp)
    
    return data

def main() -> int:
    if len(sys.argv) != 4:
        print(f'Usage: {sys.argv[0]} <repeat> <input_json> <output_json>')
        return 1
    try:
        repeat = int(sys.argv[1])
    except ValueError:
        print(f'{sys.argv[1]} is not a valid integer')
        return 1
    if not os.path.exists(sys.argv[2]):
        print(f'{sys.argv[2]} does not exist')
        return 1
    filename = 'tmp.json'
    base = init_json(sys.argv[2])
    # Create some regular expressions
    # Cache-Miss
    WORST_CM = 0
    CM = re.compile(r'.*(\d+).*(cache-misses).*')
    # L1-dcache-load-misses
    WORST_L1DLM = 0
    L1DLM = re.compile(r'.*(\d+).*(L1-dcache-load-misses).*')
    # L1-icache-load-misses
    WORST_L1ILM = 0
    L1ILM = re.compile(r'.*(\d+).*(L1-icache-load-misses).*')
    # Cache-Reference
    WORST_CR = 0
    CR = re.compile(r'.*(\d+).*(cache-references).*')
    # Bus-Cycle
    WORST_BC = 0
    BC = re.compile(r'.*(\d+).*(bus-cycles).*')
    # L1-dcache-loads
    WORST_L1DL = 0
    L1DL = re.compile(r'.*(\d+).*(L1-dcache-loads).*')
    # L1-icache-loads
    WORST_L1IL = 0
    L1IL = re.compile(r'.*(\d+).*(L1-icache-loads).*')
    # L1-dcache-store-misses
    WORST_L1DSM = 0
    L1DSM = re.compile(r'.*(\d+).*(L1-dcache-store-misses).*')
    # bus-access, but only for ARM-A72
    WORST_BA = 0
    BA = re.compile(r'.*(\d+).*(armv8_cortex_a72/bus_access/).*')
    
    res = {
        'cache-misses': (CM, WORST_CM),
        'L1-dcache-load-misses': (L1DLM, WORST_L1DLM),
        'L1-icache-load-misses': (L1ILM, WORST_L1ILM),
        'cache-references': (CR, WORST_CR),
        'bus-cycles': (BC, WORST_BC),
        'L1-dcache-loads': (L1DL, WORST_L1DL),
        'L1-icache-loads': (L1IL, WORST_L1IL),
        'L1-dcache-store-misses': (L1DSM, WORST_L1DSM),
        'bus-access': (BA, WORST_BA),
    }
    
    for i in range(repeat):
        p = subprocess.Popen(
            # ['./driver', '1', sys.argv[2], filename],
            # perf stat -e cache-misses -e L1-dcache-load-misses -e L1-icache-load-misses -e cache-references -e bus-cycles -e bus-access
            # -e L1-dcache-loads -e L1-icache-loads -- ./driver 1 test/CoreInfo.json output.json
            [
                'perf', 'stat', 
                '-e', 'cache-misses', 
                '-e', 'L1-dcache-load-misses', 
                '-e', 'L1-icache-load-misses', 
                '-e', 'cache-references', 
                '-e', 'bus-cycles', 
                '-e', 'L1-dcache-store-misses', 
                '-e', 'L1-dcache-loads', 
                '-e', 'L1-icache-loads', 
                '-e', 'armv8_cortex_a72/bus_access/', 
                '--', 
                './driver', '1', sys.argv[2], sys.argv[3],
            ],
            env = {'LD_LIBRARY_PATH': '.'},
            stderr = subprocess.PIPE
        )
        p.wait()
        
        # Get RETVAL
        ret = p.returncode
        
        if ret != 0:
            print(f'Error at cycle({i}): {ret}')
            print(f'Command: {" ".join(p.args)}')
            print(f'STDERR: {p.stderr.read().decode("utf-8")}')
            return ret

        outputs = p.stderr.read().decode('utf-8').split('\n')
        
        for line in outputs:
            # Do some regex
            for key in res:
                m = res[key][0].match(line)
                if m is not None:
                    result = int(m.group(1))
                    if result > res[key][1]:
                        pt = res[key][0]
                        res[key] = (pt, result)
        
        base = process_json(filename, base, i)
    
    with open(sys.argv[3], 'w') as f:
        json.dump(base, f, indent = 4)
        
    for key in res:
        print(f'Worst {key}: {res[key][1]}')
    
    return 0

if __name__ == '__main__':
    sys.exit(main())