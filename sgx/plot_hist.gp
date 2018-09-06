reset

if (!exists("infile"))  infile='latency_bench.txt'
if (!exists("outfile")) outfile='hist.pdf'

n=70       # number of intervals

min= 7600.
max= 8600.

width=(max-min)/n #interval width
# function used to map a value to the intervals
hist(x,width)=width*floor(x/width)+width/2.0

set terminal pdf
set output outfile
set xrange [min:max]
set yrange [0:80]

#to put an empty boundary around the
#data inside an autoscaled graph.
set offset graph 0.05,0.05,0.05,0.0
set xtics min,(max-min)/5,max
set boxwidth width*0.9
set style fill solid 0.5 #fillstyle
set tics out nomirror
set xlabel "IRQ Latency"
set ylabel "Frequency"

#count and plot
plot infile u (hist($1,width)):(1.0) smooth freq w boxes lc rgb"green" notitle
