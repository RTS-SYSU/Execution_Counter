#!/usr/bin/env python3

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
    for i in range(repeat):
        p = subprocess.Popen(
            ['./driver', '1', sys.argv[2], filename],
            env = {'LD_LIBRARY_PATH': '.'},
        )
        p.wait()
        
        base = process_json(filename, base, i)
    
    with open(sys.argv[3], 'w') as f:
        json.dump(base, f, indent = 4)
    return 0

if __name__ == '__main__':
    sys.exit(main())