#!/usr/bin/gnuplot
load "../../common.plot"

font_type = "Helvetica,35"
font_type_bold = "Helvetica-Bold,35"

set terminal pdf dl 1.5 enhanced dashed size 6.0, 5 font font_type
set output "eval-imbalance-traces.pdf"

set style fill solid 2 border lt -1
set datafile separator " "
set style data errorbars

### Variables
points_lw = 4.5
points_size = 2
line_width = 2

x_start = 0
x_end = 5
x_offset = 0.2
y_min = 0
y_max = 165

### Input files
input_file_1 = "kth-imbalance.csv"
input_file_2  = "caida-imbalance.csv"

### Margins
bm_bottom = 0.17
tm_bottom = 0.98
bm_top = 0.26
tm_top = 0.984
lm = 0.199
rm = 0.99

set multiplot layout 1,2 columnsfirst upwards

# Margins
set bmargin at screen bm_bottom
set tmargin at screen tm_bottom
set lmargin at screen lm
set rmargin at screen rm

# Leave the right border open
set border 1+2+4+8 lw 2

# X-axis
set xlabel "Traffic Rate (Mpps)"
set xlabel font font_type_bold
set xlabel offset 0,0.41
set xrange [x_start:x_end + x_offset]
#set xtics mirror 1
# set logscale x 2
set xtics 1

# Grid
set grid xtics lw 3.0 lt 0
set grid ytics lw 3.0 lt 0

# Legend
unset key

# Y-axis
set ylabel "{/Helvetica:Bold Load Imbalance (\%)}"
set ylabel offset 0.8,-0.5
set yrange [y_min:y_max]
set ytics border in scale 1,0.5 norotate mirror
# set ytics mirror 0.010
set ytics 20

# Legend
set key outside opaque bottom Right title
set border back
set key box linestyle 1 lt rgb(black)
set key spacing 1.1 font ",34.0"
set key vertical maxrows 3
set key width 1.5
set key height 0.5
set key samplen 3.0
# set key at 31,68.3
# set key invert
set key top right inside
set key reverse Left

### Linestyles
set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#7bccc4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#78c679'

# Highlight the difference
# set arrow heads back filled from 16.35,104 to 16.35,216.5 ls 1 linewidth 3
# set label '-54%' at 15.16,166 textcolor ls 1
# set arrow nohead from 1.3,217 to 16.4,217 ls 1 linewidth 3 dt 2

# set label '2x' at 84,60 textcolor rgb 'red'
# set arrow heads from 90,42 to 90,79 ls 1 linewidth 3 lc rgb 'red' dt 1


plot \
	input_file_1  using ($1/1000):($9-100):(($4)-100):(($6)-100) with errorbars ls 4 title "Campus",\
	input_file_2  using ($1/1000):(($9)-100):(($4)-100):(($6)-100) with errorbars ls 1 title "CAIDA",\
        input_file_1  using ($1/1000):(($9)-100) with lines ls 4 dt 5 notitle,\
	input_file_2  using ($1/1000):(($9)-100) with lines ls 1 dt 4 notitle
unset multiplot
