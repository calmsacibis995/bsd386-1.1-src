tcc -c -w -K -G -a -d -O -Z zipinfo.c envargs.c match.c
tcc -c -w -K -G -a -d -O -Z -DZIPINFO -omisc_.obj misc.c
tcc -K -G -a -d -O -Z zipinfo.obj envargs.obj match.obj misc_.obj
rem del zipinfo.obj
rem del match.obj
rem del misc_.obj
rem del envargs.obj
