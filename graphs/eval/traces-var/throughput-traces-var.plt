#!/usr/bin/gnuplot

### Input file
baseline_file = "fc-data.dat"
ref_file = "FAJITA-data.dat"
y_scale=1


points_lw = 2
points_size = 1
line_width = 2

### Margins
bm_bottom = 0.18
tm_bottom = 0.98
lm = 0.12
rm = 0.98

font_type = "Helvetica,28"
font_type_bold = "Helvetica-Bold,28"

set terminal pdf color enhanced font "Helvetica,38" size 10cm,4.5cm;
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
set key vertical maxrows 2
set key width 0
set key height 0.2
set key samplen 3.0
set key at 0.857, 67.35
set key font "Helvetica, 15"
set key invert 
# set key top left inside
#set key bottom Left left reverse box width 2
set xtics font "Helvetica, 15" 
set ytics font "Helvetica, 15"
# X-axis
set xlabel "Workload" font "Helvetica-Bold,15"
set xlabel offset 0,1.75
set xtics offset 0,0.7 nomirror
set xtics border in scale 1,0.5 norotate autojustify mirror
set xrange [-0.5:2.5]

# Y-axis
set ylabel "Throughput (Mpps)" font "Helvetica-Bold,15"
set ylabel offset 3.6,0
set yrange [0:99]
set ytic 20
set ytics offset 0.7,0 nomirror
set tic scale 0.2

set grid

set style data histogram
set style histogram cluster gap 1 
set xtics format ""
# set terminal pdf size 10cm,10cm
set output 'traces-var-throughput.pdf'

set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#78c679'

plot baseline_file using 3:xtic(2) ls 1 title "FastClick+BR", \
ref_file using 3 with histogram ls 4 title "FAJITA"
