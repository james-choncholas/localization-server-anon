#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# short run
$scriptpath/../build/bin/emp_float_server_convergence gn 3 2 12 | tee $scriptpath/../results/gn_emp_float_server_benchmark_short.log
$scriptpath/../build/bin/emp_float_server_convergence lm 3 2 12 | tee $scriptpath/../results/lm_emp_float_server_benchmark_short.log

# large run
$scriptpath/../build/bin/emp_float_server_convergence gn 3 2 256 | tee $scriptpath/../results/gn_emp_float_server_benchmark_long.log
$scriptpath/../build/bin/emp_float_server_convergence lm 3 2 256 | tee $scriptpath/../results/lm_emp_float_server_benchmark_long.log