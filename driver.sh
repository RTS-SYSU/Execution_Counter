#!/bin/bash

# Takes 4 arguments
# args[0] = repeat count
# args[1] = path to the directory containing the json files
# args[2] = path to the directory containing the output files

print_help() {
    echo "Usage: $0 <repeat count> <path to json files> <path to the libs> <path to output files>"
}

if [ "$#" -ne 4 ]; then
    echo "Illegal number of parameters"
    print_help
    exit 1
fi

echo "INFO: Running $0"

echo "Repeats: $1"
echo "Json files path: $2"
echo "Dylib files path: $3"
echo "Output files path: $4"

# echo "End of INFO"

echo
echo

jsons=$(ls $2 | grep .json$)
# echo $jsons

# Setting up environment variables
export LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH

mkdir -p $4

for json in $jsons
do
    echo "INFO: Running $2/$json"
    LD_LIBRARY_PATH=$3:$LD_LIBRARY_PATH ./driver $1 $2/$json $4/$json
done