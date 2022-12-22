#! /bin/sh
# teachjove

cp /usr/contrib/lib/jove/teach-jove $HOME/teach-jove || exit 1
exec jove $HOME/teach-jove
