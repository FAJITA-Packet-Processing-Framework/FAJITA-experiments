# Set the terminal type
set terminal pngcairo enhanced font 'Arial,10' fontscale 1.0 size 800,600

# Set output file
set output 'plot.png'

# Set titles and labels
set title "Flow vs Throughput and Imbalance"
set xlabel "Flows"
set ylabel "Imbalance"
set y2label "Throughput"

# Set grid
set grid

# Enable the second y-axis
set y2tics

# Set styles for the lines and points
set style line 1 lt 1 lw 2 pt 7 ps 1.5
set style line 2 lt 2 lw 2 pt 5 ps 1.5

# Plot the data
plot 'results.csv' using 1:3 with linespoints ls 1 title 'Imbalance' axes xy1, \
     'results.csv' using 1:2 with linespoints ls 2 title 'Throughput' axes xy2

