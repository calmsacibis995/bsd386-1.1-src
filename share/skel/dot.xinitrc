#!/bin/sh

# xsetroot -solid \#667 2>/dev/null
# make bell audible
# xset b 100 880
xset b off
# xset m 4
# xset bc

# xset +fp /usr/X11/lib/fonts/100dpi
# xset +fp /usr/X11/lib/fonts/75dpi
# xset +fp /usr/X11/lib/fonts/misc
# xset +fp /usr/X11/lib/fonts/lucida
# xset +fp /usr/X11/lib/fonts/mac
# xset +fp /usr/X11/lib/fonts/sun

# load my resources
xrdb -load < $HOME/.Xresources

# adjust key bindings
xmodmap $HOME/.Xmodmaprc

# enable hosts
xhost liberty staring redrock mjk-test >/dev/null

# start up a window manager
twm &

[ -f X1 ] && one=1

case $SHELL in
    *ksh)
	arg=-l
	;;
esac

#xterm -name console -ls -C -geom 80x8-2-110 -fn 6x13 -display unix:0 -e xstart $one exec $SHELL $arg
#exec xterm -C -fn 9x15 -geometry 80x24+0+0 -n Console -T Console -ls
exec xterm -C -geometry 80x12-2-110 -fn 6x13 -name console -ls +ut -e xstart $one exec $SHELL $arg
