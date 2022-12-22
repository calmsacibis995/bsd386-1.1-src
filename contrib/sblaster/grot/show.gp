set terminal postscript default mono "Times-Roman" 12
set output "out.ps"

set title "Audio Position vs Velocity" 0,-2

#plot [-130:130] [-260:260] "out.dat" with dots
#plot [-130:130] [-10:10] "out.dat" with lines
plot "out.dat" with lines
pause -1 "Done."