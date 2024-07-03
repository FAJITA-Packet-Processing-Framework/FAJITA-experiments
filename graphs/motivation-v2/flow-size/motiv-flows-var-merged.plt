#!/usr/bin/gnuplot

### Input file
baseline_file = "8cores/OFRXPPS.csv"
llc_file = "8cores/NFLLCMISSESPP.csv"
y_scale=1

points_lw = 2
points_size = 1
line_width = 2

### Margins
bm_bottom = 0.17
tm_bottom = 0.98
lm = 0.12
rm = 0.88

font_type = "Helvetica,16"
font_type_bold = "Helvetica-Bold,16"

set terminal pdf color enhanced font "Helvetica,38" size 10cm,5cm;
set style fill solid 1 border -1 
#set style boxplot nooutliers
#set style boxplot fraction 1.00
#set style data boxplot
#set boxwidth 1
# set size square

# Margins
set bmargin at screen bm_bottom
set tmargin at screen tm_bottom
set lmargin at screen lm
set rmargin at screen rm

# Legend
set key outside opaque bottom Right title
#set key top right opaque
set border back
set key box linestyle 1 lt rgb("#000000")
set key vertical maxrows 4
set key width 0
set key height 0.5
set key samplen 2.0
set key at 5100, 110
set key font "Helvetica, 15"

#set key bottom Left left reverse box width 2
set xtics font "Helvetica, 15" 
set ytics font "Helvetica, 15"
set y2tics font "Helvetica, 15"
# X-axis
set xlabel "Total Number of Flows (k)" font "Helvetica-Bold,16"
set xlabel offset 0,1.75
set xtics offset 0,0.7 nomirror
set xtics border in scale 1,0.5 norotate autojustify mirror
set xrange [16:5390]
set xtic 4
set logscale x 2

# Y-axis
set ylabel "Throughput (Mpps)" font "Helvetica-Bold,16"
set ylabel offset 4.6,0
set yrange [0:158]
set ytic 30
set ytics offset 0.7,0 nomirror
set tic scale 0.2

# Y2-axis
set y2label "LLC Misses / packet" font "Helvetica-Bold,15"
set y2label offset -5.0,0
set y2range [0:2.945]
set y2tics 0.4
set y2tics offset -1,0 nomirror


set grid

set style data histogram
set style histogram cluster gap 1 
# set xtics format ""
# set terminal pdf size 10cm,10cm
set output 'motiv-flow-var-throughput-llc-misses.pdf'

set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#0868ac'

plot baseline_file using ($1*64/1000):($5/1000000):($4/1000000):($6/1000000) with errorbars ls 1 title "Throughput",\
     baseline_file using ($1*64/1000):($5/1000000) with lines ls 1 dt 2 notitle,\
     llc_file using ($1*64/1000):($5 * 8):($3 * 8):($7 * 8) with errorbars ls 3 title "LLC misses" axis x1y2,\
     llc_file using ($1*64/1000):($5 * 8) with lines ls 3 dt 2 notitle axis x1y2
unset multiplot
