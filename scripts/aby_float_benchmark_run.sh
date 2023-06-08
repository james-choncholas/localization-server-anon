#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd $scriptpath/../build/bin

echo aby tests require loop leak, not sisl in privacy conf
sleep 1

killall gn_aby_float_server_benchmark
killall lm_aby_float_server_benchmark
./gn_aby_float_server_benchmark 0 3 12 > $scriptpath/../results/gn_aby_float_server_benchmark_short.log &
./gn_aby_float_server_benchmark 1 3 12 

./lm_aby_float_server_benchmark 0 3 12 > $scriptpath/../results/lm_aby_float_server_benchmark_short.log &
./lm_aby_float_server_benchmark 1 3 12 
