#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir=$scriptpath/../results/
plotdir=$scriptpath/../plots/

#SHOW="--show"
SHOW=""

# relies on:
#   emp_float_benchmark_run.sh

# NOTE: For GB you need to modify plotter script

python3 $scriptpath/../scripts/plotter.py \
     --csvlog \
        "$logdir/gn_emp_float_eth3d_bench_long.log" \
        "$logdir/lm_emp_float_eth3d_bench_long.log" \
     --graphpath "$plotdir/netio.pdf" \
     --title "Network IO per
Localization Iteration" \
     --only-tags \
        "gn_bytes_rx_normalized" \
        "lm_bytes_rx_normalized" \
     --custom-legend-labels \
        "GN" \
        "LM" \
     --xlabel "Number of Features" \
     --ylabel "Bytes per Iteration" \
     --fig-w 4 \
     --fig-h 3 \
     --color-theme 'dracula' \
     $SHOW
