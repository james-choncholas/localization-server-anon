#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir=$scriptpath/../results/
plotdir=$scriptpath/../plots/

#SHOW="--show"
SHOW=""

python3 $scriptpath/../scripts/plotter.py \
    --csvlog \
       `#"$logdir/gn_emp_float_server_benchmark_short.log"` \
       "$logdir/lm_emp_float_server_benchmark_short.log" \
    --graphpath "$plotdir/emp_arith_ops.pdf" \
    --title "Arithmetic Operations Count" \
    --only-tags \
        `#"gn_additions"` \
        `#"gn_subtractions"` \
        `#"gn_multiplications"` \
        `#"gn_divisions"` \
        "lm_additions" \
        "lm_subtractions" \
        "lm_multiplications" \
        "lm_divisions" \
    --custom-legend-labels \
        "Add" \
        "Subract" \
        "Multiply" \
        "Divide" \
    --xlabel "Number of Input Points" \
    --ylabel "Number of Operations" \
    --fig-w 4 \
    --fig-h 4 \
    --horizontal \
    $SHOW
    #--color-theme "dracula" \
    #--only-tags "opencv_error_vs_numpts" \
    #--custom-legend-labels "opencv (float)" \
    #   "float" \
    #   "fixed (64bit)" \
