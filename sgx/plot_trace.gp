reset

if (!exists("infile")) infile='latency_trace.txt'

set terminal pdfcairo size 20cm,10cm
set output 'trace.pdf'

set xrange [0:560]
set yrange [7800:8600]

set tics out nomirror
set boxwidth 0.5
set style fill solid 0.5
set xlabel "Time (instruction number)"
set ylabel "IRQ Latency"

plot infile u 0:1 smooth freq w boxes lc rgb"red" notitle
