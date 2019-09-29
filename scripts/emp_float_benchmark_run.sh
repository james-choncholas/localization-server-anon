#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# short run
killall gn_emp_float_server_benchmark
killall lm_emp_float_server_benchmark
$scriptpath/../build/bin/gn_emp_float_server_benchmark 0 10 12 > $scriptpath/../results/gn_emp_float_server_benchmark_short.log &
$scriptpath/../build/bin/gn_emp_float_server_benchmark 1 10 12

$scriptpath/../build/bin/lm_emp_float_server_benchmark 0 10 12 > $scriptpath/../results/lm_emp_float_server_benchmark_short.log &
$scriptpath/../build/bin/lm_emp_float_server_benchmark 1 10 12


# full long run
killall gn_emp_float_server_benchmark
killall lm_emp_float_server_benchmark
$scriptpath/../build/bin/gn_emp_float_server_benchmark 0 1 256 > $scriptpath/../results/gn_emp_float_server_benchmark_long.log &
$scriptpath/../build/bin/gn_emp_float_server_benchmark 1 1 256

$scriptpath/../build/bin/lm_emp_float_server_benchmark 0 1 256 > $scriptpath/../results/lm_emp_float_server_benchmark_long.log &
$scriptpath/../build/bin/lm_emp_float_server_benchmark 1 1 256

