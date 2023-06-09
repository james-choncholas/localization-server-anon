#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Set latency in network_setup.sh then run it"
sleep 1

# long run only
$scriptpath/../build/bin/emp_float_server_convergence gn 3 2 256 | tee $scriptpath/../results/gn_emp_float_server_benchmark_long_latency.log
$scriptpath/../build/bin/emp_float_server_convergence lm 3 2 256 | tee $scriptpath/../results/lm_emp_float_server_benchmark_long_latency.log

sed -i 's/emp_float_gn_time_vs_points_per_loc_itr/emp_float_gn_time_vs_points_per_loc_itr_latency/g' $scriptpath/../results/gn_emp_float_server_benchmark_long_latency.log
sed -i 's/emp_float_lm_time_vs_points_per_loc_itr/emp_float_lm_time_vs_points_per_loc_itr_latency/g' $scriptpath/../results/lm_emp_float_server_benchmark_long_latency.log
