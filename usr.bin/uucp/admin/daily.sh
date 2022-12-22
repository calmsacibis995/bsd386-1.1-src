#!/bin/sh

PATH=/bin:/usr/bin

# vixie 28jan94 [original]
#
# $Id: daily.sh,v 1.2 1994/01/31 04:42:06 donn Exp $

# if we had a uupoll command, this would be an ideal place to use it on
# whatever systems we want to poll once a day just to keep the pipes clear.

echo ""
uusnap

exit 0
