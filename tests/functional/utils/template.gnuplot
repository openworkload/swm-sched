set format cb "%4.1f"

set xlabel "jobs"
set ylabel "nodes"
set zlabel "t" offset -1,0

set xtics JOBSTEP
set ytics NODESTEP

set key off
set title "TITLE"
set output "OUTPUTFILE"
set terminal png size 1400,800 enhanced font "Helvetica,20"
set timestamp

set dgrid3d
set hidden3d
splot "INPUTFILE" u 1:2:3 with lines
