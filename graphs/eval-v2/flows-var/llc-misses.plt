#!/usr/bin/gnuplot
load "../../common.plot"

font_type = "Helvetica,42"
font_type_bold = "Helvetica-Bold,42"

set terminal pdf dl 1.5 enhanced dashed size 7, 6 font font_type
set output "eval-llc-misses-flows-var-square.pdf"

set style fill solid 2 border lt -1
set datafile separator " "
set style data errorbars

### Variables
points_lw = 6
points_size = 3
line_width = 2

x_start = 16
x_end = 6290
x_offset = 0
y_min = 0
y_max = 2.1

### Input files
fajita_file  = "csvs/fajitaNFLLCMISSESPP.csv"
fastclick_file  = "csvs/fastclickNFLLCMISSESPP.csv"

### Margins
bm_bottom = 0.17
tm_bottom = 0.98
bm_top = 0.26
tm_top = 0.984
lm = 0.19
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
set xlabel "Total Number of Flows (k)"
set xlabel font font_type_bold
set xlabel offset 0,0.34
set xrange [x_start:x_end + x_offset]
set xtic 4
set logscale x 2

# Grid
set grid xtics lw 3.0 lt 0
set grid ytics lw 3.0 lt 0

# Legend
unset key

# Y-axis
set ylabel "{/Helvetica:Bold LLC Misses / Packet}"
set ylabel offset 1.27,0
set yrange [y_min:y_max]
set ytics border in scale 1,0.5 norotate mirror
# set ytics mirror 0.010
set ytics 0.4

# Legend
set key outside opaque top left title
set border back
set key box linestyle 1 lt rgb(black)
set key spacing 1.1 font ",32.0"
set key vertical maxrows 4
set key width 0
set key height 0.2
set key samplen 2.0
# set key at 2750,0.1
set key inside top left
set key invert
set key reverse Left
set key font ",40"

### Linestyles
set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#78c679'

# Highlight the difference
# set arrow heads back filled from 16.35,104 to 16.35,216.5 ls 1 linewidth 3
# set label '-54%' at 15.16,166 textcolor ls 1
# set arrow nohead from 1.3,217 to 16.4,217 ls 1 linewidth 3 dt 2

plot \
        fajita_file using ($1*64/1000):($5 * 8):($3 * 8):($7 * 8) with errorbars ls 1 title "FAJITA",\
        fajita_file  using ($1*64/1000):($5 * 8) with lines ls 1 dt 5 notitle,\
        fastclick_file  using ($1*64/1000):($5 * 8):($3 * 8):($7 * 8) with errorbars ls 4 title "FastClick+BR",\
        fastclick_file using ($1*64/1000):($5 * 8) with lines ls 4 dt 5 notitle
unset multiplot
