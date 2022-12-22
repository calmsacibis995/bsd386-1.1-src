$set 5 #Aliases
$quote "
$ #Delete
1	"Delete 1 alias? (%c/%c) "
$ #DeletePlural
2	"Delete %d aliases? (%c/%c) "
$ #KeepDelete
3	[Keeping 1 alias and deleting %d.]
$ #KeepDeletePlural
4	[Keeping %d aliases and deleting %d.]
$ #DeleteAll
5	[Deleting all aliases.]
$ #Select
7	Alias mode: %d shown out of %d %s[ELM %s]
$ #Single
8	Alias mode: 1 alias %s[ELM %s]
$ #Plural
9	Alias mode: %d aliases %s[ELM %s]
$ #RMenuLn1
10	You can use any of the following commands by pressing the first character;
$ #RMenuLn2
11	a)lias current message, n)ew alias, d)elete or u)ndelete an alias,
$ #RMenuLn3
12	m)ail to alias, or r)eturn to main menu.  To view an alias, press <return>.
$ #RMenuLn4
13	j = move down, k = move up, ? = help
$ #MenuLn1
14	Alias commands:  ?=help, <n>=set current to n, /=search pattern
$ #MenuLn2
15	a)lias current message, c)hange, d)elete, e)dit aliases.text, f)ully expand,
$ #MenuLn3
16	l)imit display, m)ail, n)ew alias, r)eturn, t)ag, u)ndelete, or e(x)it
$ #Sort
17	Sorting aliases by %s...
$ group and person must both be 6 characters long
$ #Group
18	" Group"
$ #Person
19	Person
$ systemflag must be 3 characters long
$ #SystemFlag
20	(S)
$ All of the 'Pad' entries below must be the same length
$ #RevAliasPad
21	"Reverse Alias Name      "
$ #RevTextPad
22	"Reverse Text File       "
$ #RevFullPad
23	"Reverse Full (Real) Name"
$ #AliasPad
24	"Alias Name              "
$ #FullPad
25	"Full (Real) Name        "
$ #TextPad
26	"Text File               "
$ #RevAliasAbr
40	Reverse-Alias
$ #RevAliasName
41	Reverse Alias Name
$ #RevFullAbr
42	Reverse-Name
$ #RevFullName
43	Reverse Full (Real) Name
$ #RevTextAbr
44	Reverse-Text
$ #RevTextFile
45	Reverse Text File
$ #AliasAbr
46	Alias
$ #AliasName
47	Alias Name
$ #FullAbr
48	Name
$ #FullName
49	Full (Real) Name
$ #TextAbr
50	Text
$ #TextFile
51	Text File
$ #EnterAliasName
52	"Enter alias name: "
$ #AlreadyGroup
54	Already a group with name %s.
$ #AlreadyAlias
55	Already an alias for %s.
$ #AcceptNew
56	"      Accept new alias? (%c/%c) "
$ #Superceed
57	"Superceed? (%c/%c) "
$ #EnterLastName
58	"Enter last name for %s: "
$ #EnterAddress
59	"Enter address for %s: "
$ #NoAddressSpec
60	No address specified!
$ #NoMessage
61	No message to alias to!
$ #CurrentMessage
62	"Current message address aliased to: "
$ #SystemAlias
63	System (%6s) alias for %s.
$ #CouldntOpenAdd
64	Couldn't open %s to add new alias!
$ #CouldntWrite
65	Couldn't write alias to file %s!
$ #CouldntOpenDelete
66	Couldn't open %s to delete alias!
$ #CouldntOpenTemp
67	Couldn't open temp file %s to delete alias!
$ #CouldntWriteTemp
68	Couldn't write to temp file %s!
$ #CouldntRenameTemp
69	Couldn't rename temp file %s after deleting alias!
$ #ReplaceExisting
70	"Replace existing alias? (%c/%c) "
$ #Resync
73	Resynchronize aliases...
$ #AddCurrent
74	Add address from current message...
$ #Mail
76	Mail...
$ #AddNew
77	Add a new alias to database...
$ #AddReturn
78	Return to main menu...
$ #ReplaceCurrent
79	Replace current alias in database...
$ #NoneToReplace
80	Warning: no aliases to replace!
$ #NotInstalled
84	Warning: new aliases not installed yet!
$ #GroupAlias
85	Group alias: %-60.60s
$ #AliasedAddress
86	Aliased address: %-60.60s
$ #FullyExpanded
87	"Fully expand alias: "
$ #AliasedFull
88	Aliased address for:\t%s\n\r
$ #PressReturn
89	Press <return> to continue.
$ #NotFound
90	Not found.
$ #Updating
91	Updating aliases...
$ #ReReading
92	Processed %d aliases.  Re-reading the database...
$ #UpdatedOK
93	Aliases updated successfully.
$ #ShortKey
95	"Key: "
$ #LongKey
96	"Key you want help for: "
$ #KeyMenu
97	Press the key you want help for, '?' for a key list, or '.' to exit help
$ #HelpDollar
98	$ = Force resynchronization of aliases, processing additions and deletions.
$ #Helpv
99	v = View the address for the currently selected alias.
$ #Helpa
100	a = Add (return) address of current message to alias database.
$ #Helpd
101	d = Mark the current alias for deletion from alias database.
$ #HelpCtrlD
102	^D = Mark for deletion user aliases matching specified pattern.
$ #Helpf
103	f = Display fully expanded address of current alias.
$ #Helpl
104	l = Limit displayed aliases on the specified criteria.
$ #HelpCtrlL
105	^L = Rewrite the screen.
$ #Helpm
106	m = Send mail to the current or tagged aliases.
$ #Helpn
107	n = Add a new user alias, adding to alias database at next resync.
$ #Helpi
108	r,q,i = Return from alias menu (with prompting).
$ #HelpQ
109	R,Q,I = Return from alias menu (no prompting).
$ #Helpt
111	t = Tag current alias for further operations.
$ #HelpCtrlT
112	^T = Tag aliases matching specified pattern.
$ #Helpu
113	u = Unmark the current alias for deletion from alias database.
$ #HelpCtrlU
114	^U = Mark for undeletion user aliases matching specified pattern.
$ #HelpX
115	x = Exit from alias menu, abandoning any pending deletions.
$ #HelpNoHelp
116	That key isn't used in this section.
$ #ErrorMemory
117	\n\r\n\rCouldn't allocate enough memory! Alias #%d.\n\r\n\r
$ #item
118	alias
$ #items
119	aliases
$ #Item
120	Alias
$ #Items
121	Aliases
$ #Prompt
122	"Alias: "
$ #LimitMessagesSelected
124	%d aliases selected.
$ #LimitMessageSelected
125	1 alias selected.
$ #LimitNoMessagesSelected
126	No aliases selected.
$quote
$ #EnterLastSelected
127	Enter:{"name","alias"} [pattern] OR {"person","group","user","system"} OR "all"
$ #EnterSelected
128	Enter: {"name","alias"} [pattern] OR {"person","group","user","system"}
$ #HelpSlash
129	/ = Search for specified name or alias in list.
$ #NoneToView
130	Warning: no aliases to view!
$ #NoneToMail
131	Warning: no aliases to send mail to!
$quote "
$ #EnterFirstName
132	"Enter first name for %s: "
$ #EnterComment
133	"Enter optional comment for %s: "
$ #AddressAs
134	Messages addressed as: %s (%s)
$ #AddressTo
135	New alias: %s is '%s'.
$ #OneTagged
136	There is 1 alias tagged...
$ #ManyTagged
137	There are %d aliases tagged...
$ #CreateGroup
138	"Create group alias? (%c/%c) "
$ #Helpe
139	e = Edit the alias text file directly (will run newalias).
$ #Edit
140	Edit %s...
$ #Helpc
142	c = Change current user alias, modifying alias database at next resync.
$ #HelpT
143	T = Tag current alias and go to next alias.
$ #Modified
144	"(modified, resync needed) "
