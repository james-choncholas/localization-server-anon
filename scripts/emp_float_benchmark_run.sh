#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# short run
$scriptpath/../build/bin/emp_float_eth3d_bench gn 5 8 12 | tee $scriptpath/../results/gn_emp_float_eth3d_bench_short.log
$scriptpath/../build/bin/emp_float_eth3d_bench lm 5 8 12 | tee $scriptpath/../results/lm_emp_float_eth3d_bench_short.log

# large run
$scriptpath/../build/bin/emp_float_eth3d_bench gn 5 8 256 | tee $scriptpath/../results/gn_emp_float_eth3d_bench_long.log
$scriptpath/../build/bin/emp_float_eth3d_bench lm 5 8 256 | tee $scriptpath/../results/lm_emp_float_eth3d_bench_long.log