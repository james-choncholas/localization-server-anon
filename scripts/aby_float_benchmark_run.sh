#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd $scriptpath/../build/bin

killall gn_aby_float_server_benchmark
killall lm_aby_float_server_benchmark
./gn_aby_float_server_benchmark 0 1 12 > $scriptpath/../results/gn_aby_float_server_benchmark_short.log &
./gn_aby_float_server_benchmark 1 1 12 

./lm_aby_float_server_benchmark 0 1 12 > $scriptpath/../results/lm_aby_float_server_benchmark_short.log &
./lm_aby_float_server_benchmark 1 1 12 

# Running the long ABY takes too long
