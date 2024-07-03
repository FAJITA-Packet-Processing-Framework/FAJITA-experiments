#!/usr/bin/gnuplot

### Input file
vpp_file = "vpp-8cores.dat"
fc_file = "fc-minbatch-8cores.dat"
y_scale=1


points_lw = 1
points_size = 1
line_width = 1

### Margins
bm_bottom = 0.18
tm_bottom = 0.98
lm = 0.13
rm = 0.99

font_type = "Helvetica,28"
font_type_bold = "Helvetica-Bold,28"

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
set key vertical maxrows 2
set key width -0.1
set key height 0.3
set key samplen 1.5
set key at 2.48, 83.95
set key font "Helvetica, 14"

#set key bottom Left left reverse box width 2
set xtics font "Helvetica, 16"
set ytics font "Helvetica, 16"
# X-axis
set xlabel "Network Function(s)" font "Helvetica-Bold,16"
set xlabel offset 0,1.75
set xtics offset 0,0.7 nomirror
set xtics border in scale 1,0.5 norotate autojustify mirror
set xrange [-0.5:2.5]

# Y-axis
set ylabel "Throughput (Mpps)" font "Helvetica-Bold,16"
set ylabel offset 4.5,0
set yrange [0:118]
set ytic 20
set ytics offset 0.7,0 nomirror
set tic scale 0.2

set grid

set style data histogram
set style histogram cluster gap 1
set xtics format ""
# set terminal pdf size 10cm,10cm
set output 'vpp-vs-fc-throughput.pdf'

set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#0868ac'

plot vpp_file using 3:xtic(2) ls 1 title "VPP", \
fc_file using 3 with histogram ls 3 title "FastClick+BR", \
vpp_file using ($0-0.18):3:(sprintf("%.1f", $3)) with labels offset 0,0.3 font "Helvetica, 15" notitle, \
fc_file using ($0+0.18):3:(sprintf("%.1f", $3)) with labels offset 0,0.3 font "Helvetica, 15" notitle
