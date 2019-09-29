#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# only runs the data oblivious implementation.
# dont forget to set privacyconf.h PPL_FLOW to data oblivious.

killall gn_emp_float_server_benchmark
killall lm_emp_float_server_benchmark
$scriptpath/../build/bin/gn_emp_float_server_benchmark 0 1 12 > $scriptpath/../results/gn_emp_float_server_benchmark_dataobl_short.log &
$scriptpath/../build/bin/gn_emp_float_server_benchmark 1 1 12 

$scriptpath/../build/bin/lm_emp_float_server_benchmark 0 1 12 > $scriptpath/../results/lm_emp_float_server_benchmark_dataobl_short.log &
$scriptpath/../build/bin/lm_emp_float_server_benchmark 1 1 12 
