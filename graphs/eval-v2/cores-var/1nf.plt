#!/usr/bin/gnuplot

### Input file
fc_file = "csvs/fcbr-oneOFRXPPS.csv"
fajita_file = "csvs/fajita-oneOFRXPPS.csv"
vpp_file="csvs/vpp-oneOFRXPPS.csv"
y_scale=1

points_lw = 2
points_size = 1
line_width = 2

### Margins
bm_bottom = 0.145
tm_bottom = 0.98
lm = 0.14
rm = 0.99

font_type = "Helvetica,46"
font_type_bold = "Helvetica-Bold,46"

set terminal svg font "Helvetica,56" size 800,530;
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
set key vertical maxrows 3
set key width -1.5
set key height 0
set key samplen 1
set key at 2.32, 98.95
set key font "Helvetica, 34"
set key invert

#set key bottom Left left reverse box width 2
set xtics font "Helvetica, 34" 
set ytics font "Helvetica, 34"
# X-axis
set xlabel "{/Helvetica:Bold Available CPU Cores}" font "Helvetica-Bold,38"
set xlabel offset 0,1.75
set xtics offset 0,0.7 nomirror
set xtics border in scale 1,0.5 norotate autojustify mirror
set xrange [-0.5:4.5]

# Y-axis
set ylabel "{/Helvetica:Bold Throughput (Mpps)}" font "Helvetica-Bold,38"
set ylabel offset 4.4,0
set yrange [0:196]
set ytic 30
set ytics offset 0.7,0 nomirror
set tic scale 0.2

set grid

set style data histogram
set style histogram cluster gap 1 
set xtics format ""
# set terminal pdf size 10cm,10cm
set output 'throughput-1nf.svg'

set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#78c679'
set style line 5 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#000000'


set arrow nohead back dashtype 2 from -0.5,178 to 4.5,178 linewidth 3
set label '{/Helvetica:Bold testbed limitation}' at 1.2,187 textcolor ls 5 font "Helvetica-Bold,28"

plot fc_file using ($5/1000000):xtic(1) ls 4 title "FastClick+BR", \
vpp_file using ($5/1000000) with histogram ls 2 title "VPP", \
fajita_file using ($5/1000000) with histogram ls 1 title "FAJITA"
