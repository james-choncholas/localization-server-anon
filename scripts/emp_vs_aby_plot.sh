#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir=$scriptpath/../results/
plotdir=$scriptpath/../plots/

#SHOW="--show"
SHOW=""

# relies on:
#   aby_float_benchmark_run.sh
#   emp_float_benchmark_run.sh

python3 $scriptpath/../scripts/plotter.py \
     --csvlog \
        "$logdir/gn_aby_float_server_benchmark_short.log" \
        "$logdir/lm_aby_float_server_benchmark_short.log" \
        "$logdir/gn_emp_float_server_benchmark_short.log" \
        "$logdir/lm_emp_float_server_benchmark_short.log" \
     --graphpath "$plotdir/emp_vs_aby.pdf" \
     --title "ABY and EMP Loop Leak Runtime" \
     --only-tags \
        "aby_yao_float_gn" \
        "aby_bool_float_gn" \
        "aby_yao_float_lm" \
        "aby_bool_float_lm" \
        "emp_float_gn_time_vs_points" \
        "emp_float_lm_time_vs_points" \
     --custom-legend-labels \
        "ABY Yao GN" \
        "ABY Bool GN" \
        "ABY Yao LM" \
        "ABY Bool LM" \
        "EMP GN" \
        "EMP LM" \
     --xlabel "Number of Points" \
     --ylabel "Runtime (s)" \
     --fig-w 4 \
     --fig-h 5 \
     --log-scale \
     --horizontal \
     $SHOW
     #--color-theme "dracula" \
