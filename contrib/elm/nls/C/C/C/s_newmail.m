$set 13 #Newmail
$ #NoSubject
1	(No Subject Specified)
$ #NoPasswdEntry
2	You have no password entry!
$ #Short
3	Warning: interval set to 1 second.  I hope you know what you're doing!\n
$ #ShortPlur
4	Warning: interval set to %d seconds.  I hope you know what you're doing!\n
$ #IncommingMail
5	Incoming mail:\n
$ #ErrNoPerm
6	\nPermission to monitor "%s" denied!\n\n
$ #ErrMaxFolders
7	Sorry, but I can only keep track of %d folders.\n
$quote "
$ #InWinPriorityTo
8	"Priority to "
$ #InWinPriority
9	"Priority "
$ #InWinTo
10	"To "
$ #PriorityMail
11	"Priority mail "
$ #Mail
12	"Mail "
$ #To
13	"to "
$ #From
14	"from "
$quote
$ #ErrFstat
15	Error %d attempting fstat on "%s"
$ #ErrUsername
16	Newmail: I can't get username!\n
$ #ArgsHelp1
17	\nUsage: %s [-d] [-i interval] [-w] {folders}\n\
\targ\t\t\tMeaning\n\r\
\t -d  \tturns on debugging output\n\
\t -i D\tsets the interval checking time to 'D' seconds\n\
\t -w  \tforces 'window'-style output, and bypasses auto-background\n\n
$ #ArgsHelp2
18	folders can be specified by relative or absolute path names, can be the name\n\
of a mailbox in the incoming mail directory to check, or can have one of the\n\
standard Elm mail directory prefix chars (e.g. '+', '=' or '%%').\n\
Furthermore, any folder can have '=string' as a suffix to indicate a folder\n\
identifier other than the basename of the file\n\n
$ #ErrExpand
19	Sorry, but I couldn't expand "%s"\n
