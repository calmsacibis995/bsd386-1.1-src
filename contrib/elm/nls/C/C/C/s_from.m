$set 17 #From
$ #ForMoreInfo
1	For more information, type "%s -h"\n
$ #NoQuietVerbose
2	Can't have quiet *and* verbose!\n
$ #CantOpenDevNull
3	Can't open /dev/null for "very quiet" mode.\n
$ #NoPasswdEntry
4	You have no password entry!
$ #CouldntExpandFilename
5	%s: couldn't expand filename %s!\n
$ #NotRegularFile
6	"%s" is not a regular file!\n
$ #NoMail
7	No mail.\n
$ #CouldntOpenFolder
8	Couldn't open folder "%s".\n
$ #CouldntOpenFolderPlural
9	Couldn't open folders "%s" or "%s".\n
$ #StringNoMail
10	%s no mail.\n
$ #NoMesgInFolder
11	No messages in that folder!\n
$ #NoExplainMail
12	%s no %s mail.\n
$ #NoExplainMessages
13	No %s messages in that folder.\n
$ #StringStringMail
14	%s %s mail.\n
$ #ThereAreMesg
15	There are %s messages in that folder.\n
$ #FollowingMesg
16	%s the following mail messages:\n
$ #FolderContainsFollowing
17	Folder contains the following %s messages:\n
$quote "
$ #FolderContains
18	"Folder contains "
$ #Message
19	message
$ #MessagePlural
20	messages
$ #NoMessages
21	no messages.\n
$ #Anon
22	anonymous
$ #YouHave
23	You have
$ #Has
24	" has"
$quote
$ #Usage
25	Usage: %s [-n] [-v] [-t] [-s {new|old|read}] [filename | username] ...\n
$ #HelpTitle
26	frm -- list from and subject lines of messages in mailbox or folder\n
$ #HelpText
27	\noption summary:\n\
-h\tprint this help message.\n\
-n\tdisplay the message number of each message printed.\n\
-Q\tvery quiet -- no output is produced.  This option allows shell\n\
\tscripts to check frm's return status without having output.\n\
-q\tquiet -- only print summaries for each mailbox or folder.\n\
-S\tsummarize the number of messages in each mailbox or folder.\n\
-s status only -- select messages with the specified status.\n\
\t'status' is one of "new", "old", "unread" (same as "old"),\n\
\tor "read".  Only the first letter need be specified.\n\
-t\ttry to align subjects even if 'from' text is long.\n\
-v\tprint a verbose header.\n
$ #New
28	new
$ #Unread
29	unread
$ #Read
30	read
$ #NewAndUnread
31	new and unread
$ #NewOrUnread
32	new or unread
$ #NewAndRead
33	new and read
$ #NewOrRead
34	new or read
$ #ReadAndUnread
35	read and unread
$ #ReadOrUnread
36	read or unread
$ #Unknown
37	unknown
