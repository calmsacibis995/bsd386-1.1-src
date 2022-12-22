$set 11 #Answer
$ #QuitWord
1	quit
$ #ExitWord
2	exit
$ #DoneWord
3	done
$ #ByeWord
4	bye
$quote "
$ #MessageTo
5	"\nMessage to: "
$ #SorryNotFound
6	Sorry, could not find '%s' [%s] in list!\n
$ #CouldNotOpenWrite
7	** Fatal Error: could not open %s to write\n
$ #Caller
8	"Caller: "
$ #Of
9	"of:     "
$ #Phone
10	"Phone:  "
$ #Telephoned
11	"TELEPHONED         - "
$ #CalledToSeeYou
12	"CALLED TO SEE YOU  - "
$ #WantsToSeeYou
13	"WANTS TO SEE YOU   - "
$ #ReturnedYourCall
14	"RETURNED YOUR CALL - "
$ #PleaseCall
15	"PLEASE CALL        - "
$ #WillCallAgain
16	"WILL CALL AGAIN    - "
$ #Urgent
17	"*****URGENT******  - "
$quote
$ #EnterMessage
18	\n\nEnter message for %s ending with a blank line.\n\n
$ #ElmCommand
19	( ( %s -s "While You Were Out" %s < %s ; %s %s) & ) > /dev/null
$ #CannotHaveMoreNames
20	** Can't have more than 'FirstName LastName' as address!\n
$ #NotFoundForGroup
21	Alias %s not found for group expansion!\n
$ #RecursionTooDeep
22	Error: Get_token calls nested greater than %d deep!\n
