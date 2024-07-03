#!/usr/bin/gnuplot
load "../../common.plot"

font_type = "Helvetica,32"
font_type_bold = "Helvetica-Bold,32"

set terminal svg dl 1.5 enhanced dashed size 800, 530 font font_type
set output "eval-throughput.svg"

set style fill solid 2 border lt -1
set datafile separator " "
set style data errorbars

### Variables
points_lw = 4
points_size = 1
line_width = 2

x_start = 0
x_end = 116
x_offset = 1
y_min = 0
y_max = 115

### Input files
fc_file = "csvs/fastclickOFRXPPS.csv"
fajita_file  = "csvs/fajitaOFRXPPS.csv"
vpp_file = "csvs/vppOFRXPPS.csv"

### Margins
bm_bottom = 0.18
tm_bottom = 0.98
bm_top = 0.26
tm_top = 0.984
lm = 0.16
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
set xlabel "{/Helvetica:Bold Offered Load (Mpps)}"
set xlabel font font_type_bold
set xlabel offset 0,0.9
set xrange [x_start:x_end + x_offset]
#set xtics mirror 1
# set logscale x 2
set xtics 20 offset 0,0.4

# Grid
set grid xtics lw 1.0 lt 0
set grid ytics lw 1.0 lt 0
set tics scale 0.3
# Legend
unset key

# Y-axis
set ylabel "{/Helvetica:Bold Throughput (Mpps)}"
set ylabel offset 1.8,-0.5
set yrange [y_min:y_max]
set ytics border in scale 0.3,0.3 norotate mirror
# set ytics mirror 0.010
set ytics 16

# Legend
set key outside opaque bottom Right title
set border back
set key box linestyle 1 lt rgb(black)
set key spacing 1 font ",32.0"
set key vertical maxrows 3
set key width -1.7
set key height 0.1
set key samplen 1.0
set key at 59,70.3
#set key invert
set key reverse Left

### Linestyles
set style line 1 pointtype 6 pointsize points_size linewidth points_lw linecolor rgb '#00441b'
set style line 2 pointtype 4 pointsize points_size linewidth points_lw linecolor rgb '#238b45'
set style line 3 pointtype 8 pointsize points_size linewidth points_lw linecolor rgb '#66c2a4'
set style line 4 pointtype 10 pointsize points_size linewidth points_lw linecolor rgb '#78c679'

# Highlight the difference
# set arrow heads back filled from 16.35,104 to 16.35,216.5 ls 1 linewidth 3
# set label '7.7x' at 62,250 textcolor rgb 'red'
# set arrow heads size 3,20 filled from 60,75 to 60,390 ls 1 linewidth 3 lc rgb 'red' dt 1

# set label '2.58x' at 81,350 textcolor rgb 'red'
# set arrow heads size 3,20 filled from 90,210 to 90,475 ls 1 linewidth 3 lc rgb 'red' dt 1


plot \
	fajita_file using ($1*8/1000000):($5/1000000):($2/1000000):($8/1000000) with errorbars ls 1 title "FAJITA",\
        fajita_file using ($1*8/1000000):($5/1000000) with lines ls 1 dt 5 notitle,\
        vpp_file using ($1*8/1000000):($5/1000000):($2/1000000):($8/1000000) with errorbars ls 2 title "VPP",\
	vpp_file using ($1*8/1000000):($5/1000000) with lines ls 2 dt 5 notitle,\
        fc_file using ($1*8/1000000):($5/1000000):($2/1000000):($8/1000000) with errorbars ls 4 title "FastClick+BR",\
        fc_file using ($1*8/1000000):($5/1000000) with lines ls 4 dt 5 notitle
unset multiplot
