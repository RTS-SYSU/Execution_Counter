#!/usr/bin/env python3

import os
import subprocess
import sys

from argparse import ArgumentParser
from pathlib import Path


def logger_info(msg: str):
    # use normal print for now
    print(f'[*] {msg}')


def logger_success(msg: str):
    # change to green color
    print(f'[+] \033[92m{msg}\033[0m')


def logger_error(msg: str):
    # change to red color
    print(f'[-] \033[91m{msg}\033[0m')


PERF_EVENT_DICT = {'cycles': -1, 'cache-misses': 0, 'cache-references': 1, 'L1-icache-load-misses': 2, 'L1-dcache-load-misses': 3, 'L1-icache-loads': 4, 'L1-dcache-loads': 5,
                   'L1-dcache-store-misses': 6, 'bus-cycles': 7, 'L1-icache-prefetch-misses': 8, 'L1-dcache-prefetch-misses': 9, 'L1-icache-prefetches': 10, 'L1-dcache-prefetches': 11}

parser = ArgumentParser(description='Execution Counter Script')
parser.add_argument('-r', '--repeat', type=int, default=10000,
                    help='Number of times to repeat the execution')
parser.add_argument('-e', '--event', type=str, default='cycles',
                    choices=sorted(list(PERF_EVENT_DICT.keys()),
                                   key=lambda x: PERF_EVENT_DICT[x]),
                    help='Event to count (default: cycles, using PMU or perf_event_open())')
parser.add_argument('-j', '--json', type=str, default='./json',
                    help='Directory where the test JSON files are stored')
parser.add_argument('-o', '--output', type=str, default='./results',
                    help='Directory to store the test results')

args = parser.parse_args()

env = os.environ.copy()
env['LD_LIBRARY_PATH'] = '.'

json_dir = Path(os.path.abspath(args.json))
output_dir = Path(os.path.abspath(args.output))

if not json_dir.exists():
    logger_error(f'JSON directory {json_dir} does not exist')
    sys.exit(1)

if not output_dir.exists():
    output_dir.mkdir()

for f in output_dir.iterdir():
    f.unlink()

logger_info(f'Running {args.repeat} times with event {args.event}')
logger_info(f'Using JSON directory {json_dir}')
logger_info(f'Storing results in {output_dir}')

EVENT = PERF_EVENT_DICT[args.event]


for test_file in json_dir.iterdir():
    if not test_file.is_file():
        continue

    if test_file.suffix != '.json':
        continue

    logger_info(f'Running test {test_file}')
    test_name = test_file.stem
    output_file = output_dir / f'{test_name}.json'
    logger_info(f'Storing results in {output_file}')

    if EVENT != -1:
        p = subprocess.Popen(['./driver', str(args.repeat), str(test_file),
                             str(output_file), '-e', str(EVENT)], env=env, stderr=subprocess.PIPE)
    else:
        p = subprocess.Popen(['./driver', str(args.repeat), str(test_file),
                             str(output_file)], env=env, stderr=subprocess.PIPE)

    p.wait()

    if p.returncode != 0:
        logger_error(f'Failed to run test {test_file}')
        logger_error(f'{p.stderr.read().decode("utf-8")}')
        continue
    else:
        logger_success(f'Successfully ran test {test_file}')
