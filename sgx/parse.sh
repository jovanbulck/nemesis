#!/bin/bash

function run_r() {
    R -q -e "x <- read.csv('$1', header = F); summary(x); \
         v <- sd(x[ , 1]); names(v) <- (' Std deviation:'); print(v);"
}

if [ "$MACRO_BENCH" -eq 1 ]; then
    ./parse_zz.py
    wc -l latency_zz*
    gnuplot -e "infile='latency_zz_trace_avg.txt'" plot_trace.gp

    #run_r latency_zz.txt
    #gnuplot -e "infile='latency_zz.txt'" -e "outfile='hist_zz.pdf'" plot_hist.gp
else
    ./parse_micro.py
    wc -l latency_trace.txt latency_bench.txt
    run_r latency_bench.txt
    gnuplot plot_hist.gp
    gnuplot plot_trace.gp
fi
