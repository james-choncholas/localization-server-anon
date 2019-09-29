#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sudo tc qdisc add dev lo root netem delay 5ms # adds to egress only


# long run only
killall gn_emp_float_server_benchmark
killall lm_emp_float_server_benchmark
$scriptpath/../build/bin/gn_emp_float_server_benchmark 0 1 256 > $scriptpath/../results/gn_emp_float_server_benchmark_long_latency.log &
$scriptpath/../build/bin/gn_emp_float_server_benchmark 1 1 256

$scriptpath/../build/bin/lm_emp_float_server_benchmark 0 1 256 > $scriptpath/../results/lm_emp_float_server_benchmark_long_latency.log &
$scriptpath/../build/bin/lm_emp_float_server_benchmark 1 1 256

sudo tc qdisc del dev lo root

sed -i '%s/emp_float_gn_time_vs_points_normalized/emp_float_gn_time_vs_points_normalized_latency/g' $scriptpath/../results/gn_emp_float_server_benchmark_long_latency.log
sed -i '%s/emp_float_lm_time_vs_points_normalized/emp_float_lm_time_vs_points_normalized_latency/g' $scriptpath/../results/lm_emp_float_server_benchmark_long_latency.log
