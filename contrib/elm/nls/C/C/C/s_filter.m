$set 16 #Filter
$ #CantGetPasswdEntry
1	Cannot get password entry for this uid!
$ #Usage
2	Usage: | filter [-nrvlq] [-f rules] [-o file]\n\
\tor: filter [-c] -[s|S]\n
$ #CouldntOpenLogFile
3	filter (%s): couldn't open log file %s\n
$ #CouldntGetRules
4	filter (%s): Couldn't get rules!\n
$ #CantOpenTempFileLeave
5	Cannot open temporary file %s
$ #MessageDeleted
6	filter (%s): Message deleted\n
$ #MailingMessage
7	filter (%s): Mailing message to %s\n
$ #CantOpenTempFile2
8	filter (%s): Can't open temp file %s!!\n
$ #LoopDetected
9	filter (%s): Filter loop detected!  Message left in file %s.%d\n
$ #PopenFailed
10	filter (%s): popen %s failed!\n
$ #FromTheFilterOf
11	From: "The Filter of %s@%s" <%s>\n
$ #XFilteredBy
12	X-Filtered-By: filter, version %s\n\n
$ #BeginMesg
13	-- Begin filtered message --\n\n
$ #EndMesg
14	\n-- End of filtered message --\n
$ #CouldntCreateLockFile
15	filter (%s): Couldn't create lock file\n
$ #CantOpenMailBox
16	filter (%s): Can't open mailbox %s!\n
$ #SavedMessage
17	filter (%s): Message saved in folder %s\n
$ #CantSaveMessageToFolder
18	filter (%s): can't save message to requested folder %s!\n
$ #CantOpenTempFile3
19	filter (%s): can't open temp file %s for reading!\n
$ #ExecutingCmd
20	filter (%s): Executing %s\n
$ #BadWaitStat
21	filter (%s): Command "%s" exited with value %d\n
$ #ForkFailed
22	filter (%s): fork of command "%s" failed\n
$ #CantOpenEither
23	filter (%s): Can't open %s either!!\n
$ #CantOpenAny
24	filter (%s): I can't open ANY mailboxes!  Augh!!\n
$ #CantOpenAnyLeave
25	Cannot open any mailbox
$ #UsingEmergMbox
26	filter (%s): Using %s as emergency mailbox\n
$ #CouldntReadRules
27	filter (%s): Couldn't read filter rules file "%s"!\n
$ #CouldntMallocFirst
28	filter (%s): couldn't malloc first condition rec!\n
$ #ErrorReadingRules1
29	filter (%s): Error reading line %d of rules - badly placed "and"\n
$ #CouldntMallocNew
30	filter (%s): Couldn't malloc new cond rec!!\n
$ #ErrorReadingRules2
31	filter (%s): Error reading line %d of rules - field "%s" unknown!\n
$ #ErrorReadingRules3
32	filter (%s): Error on line %d of rules - action "%s" unknown\n
$ #CantAllocRules
33	filter (%s): Can't alloc memory for %d rules!\n
$ #ContainsNotImplemented
34	filter (%s): Error: rules based on "contains" are not implemented!\n
$ #Sender
35	<sender>
$ #ReturnAddress
36	<return-address>
$ #Subject
37	<subject>
$ #ReSubject
38	<Re: subject>
$ #DayOfMonth
39	<day-of-month>
$ #DayOfWeek
40	<day-of-week>
$ #Month
41	<month>
$ #Year
42	<year>
$ #Hour
43	<hour>
$ #Time
44	<time>
$ #ErrorTranslatingMacro
45	filter (%s): Error on line %d translating %%%c macro in word "%s"!\n
$ #Always
46	\nRule %d:  ** always ** \n\t%s %s\n
$ #RuleIf
47	\nRule %d:  if (
$ #Not
48	not %s %s %s%s%s
$ #And
$quote "
49	" and "
$quote
$ #Then
50	) then\n\t  %s %s\n
$ #Delete
51	Delete
$ #Save
52	Save
$ #CopyAndSave
53	Copy and Save
$ #Forward
54	Forward
$ #CopyAndForward
55	Copy and Forward
$ #Leave
56	Leave
$ #Execute
57	Execute
$ #Action
58	?action?
$ #CantOpenFiltersum
59	filter (%s): Can't open filtersum file %s!\n
$ #WarningInvalidForShort
60	filter (%s): Warning - rule #%d is invalid data for short summary!!\n
$ #SumTitle
61	\n\t\t\tA Summary of Filter Activity\n
$ #TotalMessagesFiltered
62	A total of %d message was filtered:\n\n
$ #TotalMsgsFlrdPlural
63	A total of %d messages were filtered:\n\n
$ #ErroneousRules
64	[Warning: %d erroneous rule was logged and ignored!]
$ #ErroneousRulesPlural
65	[Warning: %d erroneous rules were logged and ignored!]
$ #DefaultRuleMesg
66	The default rule of putting mail into your mailbox
$ #AppliedTimes
67	\n\tapplied %d time (%d%%)\n\n
$ #AppliedTimesPlural
68	\n\tapplied %d times (%d%%)\n\n
$ #LeaveMesg
69	(leave mail in mailbox)
$ #DeleteMesg
70	(delete message)
$ #SaveMesg
71	(save in "%s")
$ #SaveCcMesg
72	(left in mailbox and saved in "%s")
$ #ForwardMesg
73	(forwarded to "%s")
$ #ForwardCMesg
74	(left in mailbox and forwarded to "%s")
$ #ExecMesg
75	(given to command "%s")
$ #CantOpenFilterlog
76	filter (%s): Can't open filterlog file %s!\n
$ #ExplicitLog
77	\n\n\n%c\n\nExplicit log of each action;\n\n
$ #MailFrom
78	\nMail from
$ #About
79	about %s
$ #AddressedTo
80	\t(addressed to %s)\n
$ #DeletedMesg
81	\tDELETED
$ #SaveFailedMesg
82	\tSAVE FAILED for file "%s"
$ #SavedMesg
83	\tSAVED in file "%s"
$ #SavedAndPutMesg
84	\tSAVED in file "%s" AND PUT in mailbox
$ #ForwardedMesg
85	\tFORWARDED to "%s"
$ #ForwardedAndPutMesg
86	\tFORWARDED to "%s" AND PUT in mailbox
$ #ExecutedMesg
87	\tEXECUTED "%s"
$ #PutMesg
88	\tPUT in mailbox
$quote "
$ #ByRule
89	" by rule #%d\n"
$quote 
$ #TheDefaultAction
90	: the default action\n
$ #ForkSaveFailed
91	filter (%s): fork-and-save message failed\n\
\tsaving with current group ID\n
$quote "
$ #RuleNum
92	"Rule #%d: "
$quote 
$ #OutOfMemory
93	filter (%s): Out of memory [malloc failed]\n
$ #ExecuteAndSave
94	Execute and Save
$ #ExecCMesg
95	(left in mailbox and given to command "%s")
$ #ExecutedSMesg
96	\tEXECUTED "%s" AND PUT in mailbox
$ #CantCompileRegexp
97	filter (%s): Error: can't compile regexp: "%s"\n
$ #WholeRegsub
98	<match>
$ #RegsubOne
99	<submatch-1>
$ #RegsubTwo
100	<submatch-2>
$ #RegsubThree
101	<submatch-3>
$ #RegsubFour
102	<submatch-4>
$ #RegsubFive
103	<submatch-5>
$ #RegsubSix
104	<submatch-6>
$ #RegsubSeven
105	<submatch-7>
$ #RegsubEight
106	<submatch-8>
$ #RegsubNine
107	<submatch-9>
