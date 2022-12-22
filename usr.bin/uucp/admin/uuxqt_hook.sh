#!/bin/sh

# $Id: uuxqt_hook.sh,v 1.1 1994/01/31 01:25:42 donn Exp $

# this will be exec'd by uuxqt whenever it finishes a pass through the
# incoming transaction pool.  since rmail runs sendmail with "-odq", one
# way to get incoming mail to be unbatched is to run (or exec) sendmail
# with "-q" from this file.  if you don't do this, incoming uucp mail will
# wait for the next queue run (see sendmail startup in /etc/{rc,rc.local}
# if you want to know how often that is.)

#exec /usr/sbin/sendmail -q
