#!/usr/bin/gnuplot

### Input file
baseline_file = "data.dat"

y_scale=1

points_lw = 1
points_size = 1
line_width = 1

### Margins
bm_bottom = 0.21
tm_bottom = 0.98
lm = 0.12
rm = 0.99

font_type = "Helvetica,15"
font_type_bold = "Helvetica-Bold,15"

set terminal pdf color enhanced font "Helvetica,35" size 10cm,4.5cm;
set style fill solid 1 border -1 
set style boxplot nooutliers
set style boxplot fraction 1.00
set style data boxplot
set boxwidth 1
# set size square

# Margins
set bmargin at screen bm_bottom
set tmargin at screen tm_bottom
set lmargin at screen lm
set rmargin at screen rm

# Legend
# set key outside opaque bottom Right title
#set key top right opaque
# set border back
# set key box linestyle 1 lt rgb("#000000")
# set key horizontal maxrows 1
# set key width 1.0
# set key height 0.8
# set key samplen 3.0
# set key at 3.25, 0.95
# set key font "Helvetica, 18"

#set key bottom Left left reverse box width 2
set xtics font "Helvetica, 15" 
set ytics font "Helvetica, 15"
# X-axis
set xlabel "Processing Pipeline" font "Helvetica-Bold,15"
set xlabel offset 0,1.6
set xtics offset 0.1,0.7 nomirror
set xtics border in scale 1,0.5 norotate autojustify mirror
set xrange [-0.5:3.5]

# Y-axis
set ylabel "Throughput (Mpps)" font "Helvetica-Bold,15"
set ylabel offset 3.5,0
set yrange [0:99]
set ytic 20
set ytics offset 0.7,0 nomirror
set tic scale 0.2

set grid
set boxwidth 0.5
# set terminal pdf size 10cm,10cm
set output 'compare-all-throughput.pdf'

set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#78c679'

### Arrows and label to highlight the improvement
#set arrow heads back filled from -0.4,83 to 1.9,83 ls 1 linewidth 1
#set label '+84%' at 28.5,42 textcolor ls 1

plot baseline_file using 1:3:($3>60 ? 0x78c679 : $3> 40 ? 0X00441b : $3> 15 ? 0x238b45 : 0x66c2a4):xticlabels(2) with boxes ls 1 lc rgb var notitle, \
"" u 1:3:3 w labels offset 0,0.3 font "Helvetica,15" notitle
