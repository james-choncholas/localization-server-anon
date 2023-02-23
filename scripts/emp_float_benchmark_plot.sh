#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir=$scriptpath/../results/
plotdir=$scriptpath/../plots/

#SHOW="--show"
SHOW=""

# relies on:
#   emp_float_benchmark_run.sh

python3 $scriptpath/../scripts/insetplotter.py \
     --csvlog \
        "$logdir/gn_emp_float_server_benchmark_long.log" \
        "$logdir/lm_emp_float_server_benchmark_long.log" \
        "$logdir/gn_emp_float_server_benchmark_long_latency.log" \
        "$logdir/lm_emp_float_server_benchmark_long_latency.log" \
     --graphpath "$plotdir/emp_float_runtime_long.pdf" \
     --title "Runtime per Iteration" \
     --only-tags \
        "emp_float_gn_time_vs_points_normalized" \
        "emp_float_lm_time_vs_points_normalized" \
        "emp_float_gn_time_vs_points_normalized_latency" \
        "emp_float_lm_time_vs_points_normalized_latency" \
     --custom-legend-labels \
        "GN (0 ms)" \
        "LM (0 ms)" \
        "GN (5 ms)" \
        "LM (5 ms)" \
     --xlabel "Number of Points" \
     --ylabel "Runtime per Iteration(s)" \
     --fig-w 4 \
     --fig-h 3 \
     $SHOW
