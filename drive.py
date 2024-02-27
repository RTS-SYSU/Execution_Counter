#!/usr/bin/env python3

import re
import subprocess
import sys
import json
import os

DEBUG = os.environ.get('DEBUG', False)

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
    # Create some regular expressions used by perf
    # Cache-Miss
    # CM = re.compile(r'\s*(\d+)\s*(cache-misses).*')
    # # L1-dcache-load-misses
    # L1DLM = re.compile(r'\s*(\d+)\s*(L1-dcache-load-misses).*')
    # # L1-icache-load-misses
    # L1ILM = re.compile(r'\s*(\d+)\s*(L1-icache-load-misses).*')
    # # Cache-Reference
    # CR = re.compile(r'\s*(\d+)\s*(cache-references).*')
    # # Bus-Cycle
    # BC = re.compile(r'\s*(\d+)\s*(bus-cycles).*')
    # # L1-dcache-loads
    # L1DL = re.compile(r'\s*(\d+)\s*(L1-dcache-loads).*')
    # # L1-icache-loads
    # L1IL = re.compile(r'\s*(\d+)\s*(L1-icache-loads).*')
    # # L1-dcache-store-misses
    # L1DSM = re.compile(r'\s*(\d+)\s*(L1-dcache-store-misses).*')
    # # bus-access, but only for ARM-A72
    # BA = re.compile(r'\s*(\d+)\s*(armv8_cortex_a72/bus_access/).*')
    
    # res = {
    #     'cache-misses': (CM, int()),
    #     'L1-dcache-load-misses': (L1DLM, int()),
    #     'L1-icache-load-misses': (L1ILM, int()),
    #     'cache-references': (CR, int()),
    #     'bus-cycles': (BC, int()),
    #     'L1-dcache-loads': (L1DL, int()),
    #     'L1-icache-loads': (L1IL, int()),
    #     'L1-dcache-store-misses': (L1DSM, int()),
    #     'bus-access': (BA, int()),
    # }
    
    init = True
    
    for i in range(repeat):
        p = subprocess.Popen(
            # ['./driver', '1', sys.argv[2], filename],
            # perf stat -e cache-misses -e L1-dcache-load-misses -e L1-icache-load-misses -e cache-references -e bus-cycles -e bus-access
            # -e L1-dcache-loads -e L1-icache-loads -- ./driver 1 test/CoreInfo.json output.json
            [
                # Uncomment this if you need to use perf, else keep it
                # 'perf', 'stat', 
                # '-e', 'cache-misses', 
                # '-e', 'L1-dcache-load-misses', 
                # '-e', 'L1-icache-load-misses', 
                # '-e', 'cache-references', 
                # '-e', 'bus-cycles', 
                # '-e', 'L1-dcache-store-misses', 
                # '-e', 'L1-dcache-loads', 
                # '-e', 'L1-icache-loads', 
                # '-e', 'armv8_cortex_a72/bus_access/', 
                # '--', 
                './driver', '1', sys.argv[2], filename,
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

        # Same as above
        # outputs = p.stderr.read().decode('utf-8').split('\n')
        
        # for line in outputs:
        #     if DEBUG:
        #         print(f'STDERR: {line}')
        #     # Do some regex
        #     for key in res:
        #         m = res[key][0].match(line)
        #         if m is not None:
        #             if DEBUG:
        #                 print(f'Line: {line}')
        #                 print(f'Match: {m.group(0)}')
        #                 print(f'Val: {m.group(1)}')
        #             result = int(m.group(1))
        #             if init:
        #                 init = False
        #                 pt = res[key][0]
        #                 res[key] = (pt, result)
        #             else:
        #                 if result > res[key][1]:
        #                     pt = res[key][0]
        #                     res[key] = (pt, result)
        
        base = process_json(filename, base, i)
    
    with open(sys.argv[3], 'w') as f:
        json.dump(base, f, indent = 4)
        
    # for key in res:
    #     print(f'Worst {key}: {res[key][1]}')
    
    return 0

if __name__ == '__main__':
    sys.exit(main())