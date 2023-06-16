#!/bin/bash
scriptpath="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd $scriptpath/../build/bin

echo aby tests require loop leak, not sisl in privacy conf
sleep 1

$scriptpath/../build/bin/aby_float_eth3d_bench gn 3 2 12 | tee $scriptpath/../results/gn_aby_float_eth3d_bench_short.log
$scriptpath/../build/bin/aby_float_eth3d_bench lm 3 2 12 | tee $scriptpath/../results/lm_aby_float_eth3d_bench_short.log