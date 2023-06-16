#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir=$scriptpath/../results/
plotdir=$scriptpath/../plots/

#SHOW="--show"
SHOW=""

# relies on:
#   emp_float_benchmark_run.sh
#   emp_float_benchmark_dataobl_run.sh

sed -i 's/grouped_bar,main,emp_/xy,main,emp_/g' $scriptpath/../results/gn_emp_float_eth3d_bench_short.log
sed -i 's/grouped_bar,main,emp_/xy,main,emp_/g' $scriptpath/../results/lm_emp_float_eth3d_bench_short.log

python3 $scriptpath/../scripts/plotter.py \
     --csvlog \
        "$logdir/gn_emp_float_eth3d_bench_short.log" \
        "$logdir/lm_emp_float_eth3d_bench_short.log" \
        "$logdir/gn_emp_float_eth3d_bench_dataobl_short.log" \
        "$logdir/lm_emp_float_eth3d_bench_dataobl_short.log" \
     --graphpath "$plotdir/loopleak_vs_dataobl.pdf" \
     --title "Runtime of Data Oblivious
and Single Iteration Localization" \
     --only-tags \
        "emp_float_gn_time_vs_points_dataobl" \
        "emp_float_lm_time_vs_points_dataobl" \
        "emp_float_gn_time_vs_points" \
        "emp_float_lm_time_vs_points" \
     --custom-legend-labels \
        "GN DO" \
        "LM DO" \
        "GN SIL" \
        "LM SIL" \
     --xlabel "Number of Points" \
     --ylabel "Runtime (s)" \
     --fig-w 4 \
     --fig-h 3 \
     --log-scale \
     $SHOW
     #--color-theme "dracula" \
