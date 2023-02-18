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
        "$logdir/gn_emp_float_server_benchmark_long.log" \
        "$logdir/lm_emp_float_server_benchmark_long.log" \
     --graphpath "$plotdir/netio.pdf" \
     --title "Network IO per Iteration" \
     --only-tags \
        "gn_bytes_tx_normalized" \
        `#"gn_bytes_rx"` \
        "lm_bytes_tx_normalized" \
        `#"lm_bytes_rx"` \
     --custom-legend-labels \
        "GN tx" \
        `#"GN rx"` \
        "LM tx" \
        `#"LM rx"` \
     --xlabel "Number of Input Points" \
     --ylabel "GB per Iteration" \
     --fig-w 4 \
     --fig-h 3 \
     $SHOW
