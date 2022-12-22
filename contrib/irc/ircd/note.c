/************************************************************************
 *   IRC - Internet Relay Chat, ircd/note.c
 *   Copyright (C) 1990 Jarle Lyngaas
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *        Author: Jarle Lyngaas
 *        E-mail: jarle@stud.cs.uit.no 
 *        On IRC: Wizible
 */

#include "struct.h"
#ifdef NPATH
#include "numeric.h"
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "common.h"
#include "h.h"

#define VERSION "v1.9.1"

#define NOTE_SAVE_FREQUENCY 30 /* Frequency of save time in minutes */
#define NOTE_MAXSERVER_TIME 120 /* Max days for a request in the server */
#define NOTE_MAXSERVER_MESSAGES 2000 /* Max number of requests in the server */
#define NOTE_MAXUSER_MESSAGES 200 /* Max number of requests for each user */
#define NOTE_MAXSERVER_WILDCARDS 200 /* Max number of server toname w.cards */
#define NOTE_MAXUSER_WILDCARDS 5 /* Max number of user toname wildcards */

#define MAX_DAYS_NO_USE_SPY 31 /* No match or no user on */
#define SECONDS_WAITFOR_MODE 10
#define LAST_CLIENTS 3000 /* Max join or sign on within SECONDS_WAITFOR_MODE */    
#define BUF_LEN 256
#define MSG_LEN 512
#define REALLOC_SIZE 1024

#define FLAGS_WASOPER (1<<0)
#define FLAGS_SIGNOFF_REMOVE (1<<1)
#define FLAGS_NAME (1<<2)
#define FLAGS_FROM_REG (1<<3)
#define FLAGS_NEWS (1<<4)
#define FLAGS_ON_THIS_SERVER (1<<5)
#define FLAGS_REPEAT_UNTIL_TIMEOUT (1<<6)
#define FLAGS_ALL_NICK_VALID (1<<7)
#define FLAGS_SERVER_GENERATED_NOTICE (1<<8)
#define FLAGS_NICK_AND_WILDCARD_VALID (1<<9)
#define FLAGS_SERVER_GENERATED_DESTINATION (1<<10)
#define FLAGS_DISPLAY_IF_RECEIVED (1<<11)
#define FLAGS_DISPLAY_IF_CORRECT_FOUND (1<<12)
#define FLAGS_DISPLAY_IF_DEST_REGISTER (1<<13)
#define FLAGS_SEND_ONLY_IF_SENDER_ON_IRC (1<<14)
#define FLAGS_NO_NICKCHANGE_SPY (1<<16)
#define FLAGS_SEND_ONLY_IF_THIS_SERVER (1<<17)
#define FLAGS_SEND_ONLY_IF_DESTINATION_OPER (1<<18)
#define FLAGS_SEND_ONLY_IF_NICK_NOT_NAME (1<<19)
#define FLAGS_SEND_ONLY_IF_NOT_EXCEPTION (1<<20)
#define FLAGS_KEY_TO_OPEN_OPER_LOCKS (1<<21)
#define FLAGS_FIND_CORRECT_DEST_SEND_ONCE (1<<22)
#define FLAGS_DISPLAY_CHANNEL_DEST_REGISTER (1<<23)
#define FLAGS_DISPLAY_SERVER_DEST_REGISTER (1<<24)
#define FLAGS_PRIVATE_DISPLAYED (1<<25)
#define FLAGS_REGISTER_NEWNICK (1<<26)
#define FLAGS_NOTICE_RECEIVED_MESSAGE (1<<27)
#define FLAGS_RETURN_CORRECT_DESTINATION (1<<28)
#define FLAGS_NOT_QUEUE_REQUESTS (1<<29)
#define FLAGS_NEWNICK_DISPLAYED (1<<30)

#define DupNewString(x,y) if (!StrEq(x,y)) { MyFree(x); DupString(x,y); }  
#define MyEq(x,y) (!myncmp(x,y,strlen(x)))
#define Usermycmp(x,y) mycmp(x,y)
#define Key(sptr) KeyFlags(sptr,-1)
#define Message(msgclient) get_msg(msgclient, 'm')
#define IsOperHere(sptr) (IsOper(sptr) && MyConnect(sptr))
#define NULLCHAR ((char *)0)
#define SPY_CTRLCHAR 13

/* Using Message(msgclient) to get message part of *any* message
   cause the spy function is an ugly hack saving log in this field... - Jarle */

typedef struct MsgClient {
            char *fromnick, *fromname, *fromhost, *tonick,
                 *toname, *tohost, *message, *passwd;
            long timeout, time, flags, id;
          } aMsgClient;
  
static int note_mst = NOTE_MAXSERVER_TIME,
           note_mum = NOTE_MAXUSER_MESSAGES,
           note_msm = NOTE_MAXSERVER_MESSAGES,
           note_msw = NOTE_MAXSERVER_WILDCARDS,
           note_muw = NOTE_MAXUSER_WILDCARDS,
           note_msf = NOTE_SAVE_FREQUENCY*60,
           wildcard_index = 0,
           toname_index = 0,
           fromname_index = 0,
           max_toname,
           max_wildcards,
           max_fromname,
           m_id = 0,
           changes_to_save = 0;

static long old_clock = 0, old_clock_request = 0;
static aMsgClient **ToNameList, **FromNameList, **WildCardList;
extern aClient *client, *find_person(), *local[];
extern int highest_fd;
extern aChannel *channel;
extern char *MyMalloc(), *MyRealloc(), *myctime();
static char *ptr = "IRCnote", *note_save_filename_tmp, file_inited = 0;

static char *UserName(sptr)
aClient *sptr;
{
 if (*(sptr->user->username) != '#') return sptr->user->username;
  else return sptr->user->username+1; 
}

static int numeric(string)
char *string;
{
 register char *c = string;

 while (*c) if (!isdigit(*c)) return 0; else c++;
 return 1;
}

static char *clean_spychar(string)
char *string;
{
 static char buf[MSG_LEN]; 
 char *c, *bp = buf;
 
 for (c = string; *c; c++) if (*c != SPY_CTRLCHAR) *bp++ = *c;
 *bp = 0;
 return buf;
}

static char *myitoa(value)
long value;
{
 static char buf[BUF_LEN]; 
  
 sprintf(buf, "%d", value);
 return buf;
}

static char *relative_time(seconds)
long seconds;
{
 static char buf[20];
 char *c;
 long d, h, m;

 if (seconds < 0) seconds = 0; 
 d = seconds / (3600*24);
 seconds -= d*3600*24;
 h = seconds / 3600;
 seconds -= h*3600;
 m = seconds / 60;
 seconds -= m*60;
 sprintf(buf, "%3dd:%2dh:%2dm", d, h, m);
 c = buf; 
 while (*c) {
              if (*c == ' ') *c = '0';
              c++;
        } 
 return buf;
}

static char *mytime(value)
long value;
{
 long clock;
 time(&clock);
 return (relative_time(clock-value));
}

static char *get_msg(msgclient, field)
aMsgClient *msgclient;
char field;
{
 char *c = msgclient->message;
 static char buf[MSG_LEN], *empty_char = "";
 int t, p;
 buf[0] = 0;

 switch(field) {
     case 'n' : t = 1; break;
     case 'u' : t = 2; break;
     case 'h' : t = 3; break;
     case 'r' : t = 4; break;
     case '1' : t = 5; break;
     case '2' : t = 6; break;
     default: t = 0;
   }
 if (!t) {
           while (*c) c++;
           while (c != msgclient->message && *c != SPY_CTRLCHAR) c--;
           if (*c == SPY_CTRLCHAR) c++;
           return c;
       }
 while(t--) {
      p = 0;
      while (*c && *c != SPY_CTRLCHAR) buf[p++] = *c++;
      if (!*c) return empty_char;
      buf[p] = 0; c++;
  };
 return buf;
}       

static void update_spymsg(msgclient)
aMsgClient *msgclient;
{
 long clock, t;
 char *buf, *empty = "-", ctrlbuf[2], mbuf[MSG_LEN]; 

 time (&clock);
 mbuf[0] = 0; ctrlbuf[0] = SPY_CTRLCHAR; ctrlbuf[1] = 0;

 buf = get_msg(msgclient, 'n'); if (!*buf) buf = empty; 
 strcat(mbuf, buf); strcat(mbuf, ctrlbuf); 
 buf = get_msg(msgclient, 'u'); if (!*buf) buf = empty; 
 strcat(mbuf, buf); strcat(mbuf, ctrlbuf); 
 buf = get_msg(msgclient, 'h'); if (!*buf) buf = empty; 
 strcat(mbuf, buf); strcat(mbuf, ctrlbuf); 
 buf = get_msg(msgclient, 'r'); if (!*buf) buf = empty; 
 strcat(mbuf, buf); strcat(mbuf, ctrlbuf); 
 buf = get_msg(msgclient, '1'); if (!*buf) buf = empty; 
 strcat(mbuf, buf); strcat(mbuf, ctrlbuf); 
 strcat(mbuf, myitoa(clock-msgclient->time)); 
 strcat(mbuf, ctrlbuf); t = MSG_LEN - strlen(mbuf) - 10;
 strncat(mbuf, clean_spychar(Message(msgclient)), t);
 strcat(mbuf, "\0");
 DupNewString(msgclient->message, mbuf);
 changes_to_save = 1;
}

static char *wildcards(string)
char *string;
{
 register char *c;

 for (c = string; *c; c++) if (*c == '*' || *c == '?') return(c);
 return 0;
}

static int only_wildcards(string)
char *string;
{
 register char *c;

 for (c = string;*c;c++) 
      if (*c != '*' && *c != '?') return 0;
 return 1;
}

static char *split_string(string, field, n)
char *string;
int field, n;
{
 static char buf[MSG_LEN], *empty_char = "";
 char *c = string;
 int p, t;
 buf[0] = 0;

 while(field--) {
      p = 0; t = n;
      while (*c) {
            if (*c == ' ' && (field || !--t)) break;
            buf[p++] = *c++;
	}
      if (!*c) if (!field) {
                  buf[p] = 0; return buf;
                } else return empty_char;
      buf[p] = 0; c++;
  };
 return buf;
}

static char *wild_fromnick(nick, msgclient)
char *nick;
aMsgClient *msgclient;
{
 static char buf[BUF_LEN];
 char *msg, *c;
 
 if (msgclient->flags & FLAGS_ALL_NICK_VALID) {
     strcpy(buf, "*"); return buf;
   }
 if (msgclient->flags & FLAGS_NICK_AND_WILDCARD_VALID
     && MyEq(msgclient->fromnick, nick)) {
     strcpy(buf, nick);
     strcat(buf, "*"); return buf;
  }
 if (msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER) {
    msg = Message(msgclient);
    while (*msg == '%') { 
           msg++; c = split_string(msg, 1, 1);
           if (!mycmp(c, nick)) { 
               strcpy(buf, "*");
               return buf;
	    }
           while (*msg && *msg != ' ') msg++; if (*msg) msg++;
      }     
  }
 return NULLCHAR;
}

static int number_fromname()
{
 register int t, nr = 0;
 long clock;

 time (&clock);
 for (t = 1;t <= fromname_index; t++) 
      if (FromNameList[t]->timeout > clock
          && !(FromNameList[t]->flags & FLAGS_SERVER_GENERATED_NOTICE)
          && !(FromNameList[t]->flags & FLAGS_SERVER_GENERATED_DESTINATION)) 
       nr++;
 return nr;         
}


static int first_tnl_indexnode(name)
char *name;
{
 register int s, t = toname_index+1, b = 0, tname;
 aMsgClient *msgclient;

 if (!t) return 0;
 while ((s = (b+t) >> 1) != b) {
       msgclient = ToNameList[s];
       tname = (msgclient->flags & FLAGS_NAME) ? 1 : 0;
       if (mycmp(tname ? msgclient->toname : msgclient->tonick, name) < 0)
        b = s; else t = s;
  }
 return t;
}

static int last_tnl_indexnode(name)
char *name;
{
 register int s, t = toname_index+1, b = 0, tname;
 aMsgClient *msgclient;

 if (!t) return 0;
 while ((s = (b+t) >> 1) != b) {
       msgclient = ToNameList[s];
       tname = (msgclient->flags & FLAGS_NAME) ? 1 : 0;
       if (mycmp(tname ? msgclient->toname : msgclient->tonick, name) > 0)
        t = s; else b = s;
   }
 return b;
}

static int first_fnl_indexnode(fromname)
char *fromname;
{
 register int s, t = fromname_index+1, b = 0;

 if (!t) return 0;
 while ((s = (b+t) >> 1) != b)
       if (mycmp(FromNameList[s]->fromname,fromname)<0) b = s; else t = s;
 return t;
}

static int last_fnl_indexnode(fromname)
char *fromname;
{
 register int s, t = fromname_index+1, b = 0;

 if (!t) return 0;
 while ((s = (b+t) >> 1) != b)
       if (mycmp(FromNameList[s]->fromname,fromname)>0) t = s; else b = s;
 return b;
}

static int fnl_msgclient(msgclient)
aMsgClient *msgclient;
{
 register int t, f, l;
 aMsgClient **index_p;

 f = first_fnl_indexnode(msgclient->fromname);
 l = last_fnl_indexnode(msgclient->fromname);
 index_p = FromNameList + f;

 for (t = f; t <= l; t++) 
      if (*(index_p++) == msgclient) return(t);
 return 0;
} 

static int tnl_msgclient(msgclient)
aMsgClient *msgclient;
{
 register int t, f, l, tname;
 aMsgClient **index_p;

 if (msgclient->flags & FLAGS_NAME 
     || !wildcards(msgclient->tonick)) {
     tname = (msgclient->flags & FLAGS_NAME) ? 1 : 0;
     f = first_tnl_indexnode(tname ? msgclient->toname : msgclient->tonick);
     l = last_tnl_indexnode(tname ? msgclient->toname : msgclient->tonick);
     index_p = ToNameList + f;
  } else {
          index_p = WildCardList + 1;
          f = 1; l = wildcard_index;
      }
 for (t = f; t <= l; t++) 
      if (*(index_p++) == msgclient) return(t);
 return 0;
} 
 
static aMsgClient *new(passwd,fromnick,fromname,fromhost,tonick,
                       toname,tohost,flags,timeout,time,message)
char *passwd,*fromnick,*fromname,*fromhost,
     *tonick,*toname,*tohost,*message;
long timeout,time,flags;
{
 register aMsgClient **index_p;
 register int t;
 int allocate,first,last,n;
 aMsgClient *msgclient;
            
 if (number_fromname() > note_msm) return NULL; 
 if (fromname_index == max_fromname-1) {
    max_fromname += REALLOC_SIZE;
    allocate = max_fromname*sizeof(FromNameList)+1;
    FromNameList = (aMsgClient **) MyRealloc((char *)FromNameList,allocate);
  }
 if (wildcard_index == max_wildcards-1) {
    max_wildcards += REALLOC_SIZE;
    allocate = max_wildcards*sizeof(WildCardList)+1;
    WildCardList = (aMsgClient **) MyRealloc((char *)WildCardList,allocate);
  }
 if (toname_index == max_toname-1) {
    max_toname += REALLOC_SIZE;
    allocate = max_toname*sizeof(ToNameList)+1;
    ToNameList = (aMsgClient **) MyRealloc((char *)ToNameList,allocate);
  }

 /* Correction if corrupt format */
 if (!(flags & FLAGS_SERVER_GENERATED_NOTICE)
     && !(flags & FLAGS_SERVER_GENERATED_DESTINATION)) flags &= ~FLAGS_NAME; 
 if (!wildcards(toname) && wildcards(tonick)) flags |= FLAGS_NAME; 

 msgclient = (aMsgClient *)MyMalloc(sizeof(struct MsgClient));
 DupString(msgclient->passwd,passwd);
 DupString(msgclient->fromnick,fromnick);
 DupString(msgclient->fromname,fromname);
 DupString(msgclient->fromhost,fromhost);
 DupString(msgclient->tonick,tonick);
 DupString(msgclient->toname,toname);
 DupString(msgclient->tohost,tohost);
 DupString(msgclient->message,message);
 msgclient->flags = flags;
 msgclient->timeout = timeout;
 msgclient->time = time;
 if (flags & FLAGS_NAME || !wildcards(tonick)) {
    n = last_tnl_indexnode(flags & FLAGS_NAME ? toname : tonick) + 1;
    index_p = ToNameList+toname_index;
    toname_index++;t = toname_index-n;
    while (t--) {
                  index_p[1] = *index_p; index_p--;
      }
    ToNameList[n] = msgclient;
  } else { 
          wildcard_index++;
          WildCardList[wildcard_index] = msgclient;
     }
 first = first_fnl_indexnode(fromname);
 last = last_fnl_indexnode(fromname);
 if (!(n = first)) n = 1;
 index_p = FromNameList+n;
 while (n <= last && mycmp(msgclient->fromhost,(*index_p)->fromhost)>0) {
        index_p++;n++;
   }
 while (n <= last && mycmp(msgclient->fromnick,(*index_p)->fromnick)>=0){ 
        index_p++;n++;
   }
 index_p = FromNameList+fromname_index;
 fromname_index++;t = fromname_index-n; 
 while (t--) {
               index_p[1] = *index_p; index_p--;
    }
 FromNameList[n] = msgclient;
 changes_to_save = 1;
 msgclient->id = ++m_id;
 return msgclient;
}

static void r_code(string,fp)
register char *string;
register FILE *fp;
{
 register int v;
 register char c, *cp = ptr;

 do {
      if ((v = getc(fp)) == EOF) {
         exit(-1);       
       }
      c = v;
      *string = c-*cp;
      if (!*++cp) cp = ptr;
  } while (*string++);
}

static void w_code(string,fp)
register char *string;
register FILE *fp;
{
 register char *cp = ptr;

 do {
      putc((char)(*string+*cp),fp);
      if (!*++cp) cp = ptr;
  } while (*string++);
}

static char *ltoa(i)
register long i;
{
 static unsigned char c[20];
 register unsigned char *p = c;

  do {
      *p++ = (i&127) + 1;
       i >>= 7;
    } while(i > 0);
    *p = 0;
    return (char *) c;
}

static long atol(c)
register unsigned char *c;
{
 register long a = 0;
 register unsigned char *s = c;

 while (*s != 0) ++s;
 while (--s >= c) {
        a <<= 7;
        a += *s - 1;
   }
  return a;
}

static void init_messages()
{
 FILE *fp,*fopen();
 long timeout,atime,clock,flags;
 int allocate;
 char passwd[20], fromnick[BUF_LEN], fromname[BUF_LEN],
      fromhost[BUF_LEN], tonick[BUF_LEN], toname[BUF_LEN],
      tohost[BUF_LEN], message[MSG_LEN], buf[20];

 file_inited = 1;
 max_fromname = max_toname = max_wildcards = REALLOC_SIZE;
 allocate = REALLOC_SIZE*sizeof(FromNameList)+1;
 ToNameList = (aMsgClient **) MyMalloc(allocate);
 FromNameList = (aMsgClient **) MyMalloc(allocate);
 WildCardList = (aMsgClient **) MyMalloc(allocate); 
 note_save_filename_tmp = MyMalloc(strlen(NPATH)+6);
 sprintf(note_save_filename_tmp,"%s.tmp",NPATH);
 time(&clock);old_clock = clock;
 fp = fopen(NPATH,"r");
 if (!fp) return;
 r_code(buf,fp);note_msm = atol(buf);
 r_code(buf,fp);note_mum = atol(buf);
 r_code(buf,fp);note_msw = atol(buf);
 r_code(buf,fp);note_muw = atol(buf);
 r_code(buf,fp);note_mst = atol(buf);
 r_code(buf,fp);note_msf = atol(buf);
 for (;;) {
        r_code(passwd,fp);if (!*passwd) break;
        r_code(fromnick,fp);r_code(fromname,fp);
        r_code(fromhost,fp);r_code(tonick,fp);r_code(toname,fp);
        r_code(tohost,fp);r_code(buf,fp),flags = atol(buf);
        r_code(buf,fp);timeout = atol(buf);r_code(buf,fp);
        atime = atol(buf);r_code(message,fp);
        flags &= ~FLAGS_FROM_REG;
        if (clock > 0 && *toname != '#' && *fromname != '#')  
            new(passwd,fromnick,fromname,fromhost,tonick,toname,
                tohost,flags,timeout,atime,message);
  }
 fclose(fp);
}

static int numeric_host(host)
char *host;
{
  while (*host) {
        if (!isdigit(*host) && *host != '.') return 0;
        host++;
   } 
 return 1;
}

static int elements_inhost(host)
char *host;
{
 int t = 0;

 while (*host) {
       if (*host == '.') t++;
       host++;
    }
 return t;
}

static int valid_elements(host)
char *host;
{
 register char *c = host;
 register int t = 0, numeric = 0;

 while (*c) {
        if (!isdigit(*c) && *c != '.' && *c != '*' && *c != '?' ) break;
        c++;
   } 
 if (!*c) numeric = 1;
 c = host;
 if (numeric)
     while (*c && *c != '*' && *c != '?') {
            if (*c == '.') t++;
            c++;
	  }
  else {
        while (*c++);
        while (c != host && *c != '*' && *c != '?') { 
               if (*c == '.') t++;
               c--;
          }
    } 
 if (!t && *c != '*' && *c != '?') t = 1;
 return t;
}

static char *local_host(host)
char *host;
{
 static char buf[BUF_LEN];
 char *buf_p = buf, *host_p;
 int t, e;

 e = elements_inhost(host);
 if (e < 2) return host;
 if (!numeric_host(host)) {
     if (!e) return host;
     host_p = host;
     if (e > 2) t = 2; else t = 1;
    while (t--) { 
           while (*host_p && *host_p != '.') {
                  host_p++;buf_p++;
             }
           if (t && *host_p) { 
              host_p++; buf_p++;
	    }
      }
     buf[0] = '*';
     strcpy(buf+1, host_p);
  } else {
          host_p = buf;
          strcpy(buf,host);
          while (*host_p) host_p++;    
          t = 2;
          while(t--) {
               while (host_p != buf && *host_p-- != '.'); 
	     }
           host_p+=2;
           *host_p++ = '*';
           *host_p = 0;
       }
  return buf;
}

static int host_check(host1,host2)
char *host1,*host2;
{
 char buf[BUF_LEN];
 
 if (numeric_host(host1) != numeric_host(host2)) return 0;
 strcpy(buf,local_host(host1));
 if (!mycmp(buf,local_host(host2))) return 1;
 return 0;
}

static long timegm(tm)
struct tm *tm;
{
 static int days[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
 register long mday = tm->tm_mday-1, mon = tm->tm_mon, year = tm->tm_year-70;

 mday+=((year+2) >> 2)+days[mon];
 if (mon < 2 && !((year+2) & 3)) mday--;
 return tm->tm_sec+tm->tm_min*60+tm->tm_hour*3600+mday*86400+year*31536000;
}

static long set_date(sptr,time_s)
aClient *sptr;
char *time_s;
{
 struct tm *tm;
 long clock,tm_gmtoff;
 int t,t1,month,date,year;
 char *c = time_s,arg[3][BUF_LEN];
 static char *months[] = {
	"January",	"February",	"March",	"April",
	"May",	        "June",	        "July",	        "August",
	"September",	"October",	"November",	"December"
    }; 

 time (&clock);
 tm = localtime(&clock);
 tm_gmtoff = timegm(tm)-clock;
 tm->tm_sec = 0;
 tm->tm_min = 0;
 tm->tm_hour = 0;
 if (*time_s == '-') {
    if (!time_s[1]) return 0;
    return timegm(tm)-tm_gmtoff-3600*24*atoi(time_s+1);
  }
 for (t = 0;t<3;t++) {
      t1 = 0;
      while (*c && *c != '/' && *c != '.' && *c != '-')
             arg[t][t1++] = *c++;
      arg[t][t1] = 0;
      if (*c) c++;
  } 

 date = atoi(arg[0]);
 if (*arg[0] && (date<1 || date>31)) {
     sendto_one(sptr,"NOTICE %s :#?# Unknown date",sptr->name);
     return -1;
  }
 month = atoi(arg[1]);
 if (month) month--;
  else for (t = 0;t<12;t++) {
            if (MyEq(arg[1],months[t])) { 
                month = t;break;
             } 
            month = -1;
        }
 if (*arg[1] && (month<0 || month>11)) {
      sendto_one(sptr,"NOTICE %s :#?# Unknown month",sptr->name);
      return -1; 
  }       
 year = atoi(arg[2]);
 if (*arg[2] && (year<71 || year>99)) {
     sendto_one(sptr,"NOTICE %s :#?# Unknown year",sptr->name);
     return -1;
   }
 tm->tm_mday = date;
 if (*arg[1]) tm->tm_mon = month;
 if (*arg[2]) tm->tm_year = year;
 return timegm(tm)-tm_gmtoff;
}

static int local_check(sptr, msgclient, passwd, flags, tonick,
                   toname, tohost, time_l, id)
aClient *sptr;
aMsgClient *msgclient;
char *passwd, *tonick, *toname, *tohost;
long flags,time_l;
int id;
{
 if (msgclient->flags == flags 
     && (!id || id == msgclient->id)
     && !Usermycmp(UserName(sptr),msgclient->fromname)
     && (!mycmp(sptr->name, msgclient->fromnick)
         || wild_fromnick(sptr->name, msgclient))
     && (!time_l || msgclient->time >= time_l 
                    && msgclient->time < time_l+24*3600)
     && !matches(tonick,msgclient->tonick)
     && !matches(toname,msgclient->toname)
     && !matches(tohost,msgclient->tohost)
     && (*msgclient->passwd == '*' && !msgclient->passwd[1]
         || StrEq(passwd,msgclient->passwd))
     &&  host_check(sptr->user->host,msgclient->fromhost)) return 1;
 return 0;
}

static int send_flag(flags)
long flags;
{
  if (flags & FLAGS_RETURN_CORRECT_DESTINATION
      || flags & FLAGS_DISPLAY_IF_CORRECT_FOUND
      || flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS
      || flags & FLAGS_DISPLAY_IF_DEST_REGISTER
      || flags & FLAGS_SERVER_GENERATED_DESTINATION
      || flags & FLAGS_NOT_QUEUE_REQUESTS
      || flags & FLAGS_NEWS
      || flags & FLAGS_REPEAT_UNTIL_TIMEOUT
         && !(flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE)) return 0;
  return 1;
}

static void display_flags(flags, c, mode)
long flags;
char *c, mode;
{
 char t = 0;
 int send = 0;
 
 if (send_flag(flags)) send = 1;
 if (mode != 'q') {
     if (send) c[t++] = '['; else c[t++] = '<';
  } else c[t++] = '+';
 if (mode != 'q' && (flags & FLAGS_WASOPER)) c[t++] = 'O';
 if (flags & FLAGS_SERVER_GENERATED_DESTINATION) c[t++] = 'H';
 if (flags & FLAGS_ALL_NICK_VALID) c[t++] = 'C';
 if (flags & FLAGS_SERVER_GENERATED_NOTICE) c[t++] = 'G';
 if (flags & FLAGS_DISPLAY_IF_RECEIVED) c[t++] = 'D';
 if (flags & FLAGS_DISPLAY_IF_CORRECT_FOUND) c[t++] = 'L';
 if (flags & FLAGS_NEWS) c[t++] = 'S';
 if (flags & FLAGS_DISPLAY_IF_DEST_REGISTER) c[t++] = 'X';
 if (flags & FLAGS_DISPLAY_CHANNEL_DEST_REGISTER) c[t++] = 'J';
 if (flags & FLAGS_DISPLAY_SERVER_DEST_REGISTER) c[t++] = 'A';
 if (flags & FLAGS_REPEAT_UNTIL_TIMEOUT) c[t++] = 'R';
 if (flags & FLAGS_SIGNOFF_REMOVE) c[t++] = 'M';
 if (flags & FLAGS_NO_NICKCHANGE_SPY) c[t++] = 'U';
 if (flags & FLAGS_NICK_AND_WILDCARD_VALID) c[t++] = 'V';
 if (flags & FLAGS_SEND_ONLY_IF_THIS_SERVER) c[t++] = 'I';
 if (flags & FLAGS_SEND_ONLY_IF_NICK_NOT_NAME) c[t++] = 'Q';
 if (flags & FLAGS_SEND_ONLY_IF_NOT_EXCEPTION) c[t++] = 'E';
 if (flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS) c[t++] = 'K';
 if (flags & FLAGS_SEND_ONLY_IF_SENDER_ON_IRC) c[t++] = 'Y';
 if (flags & FLAGS_NOTICE_RECEIVED_MESSAGE) c[t++] = 'N';
 if (flags & FLAGS_RETURN_CORRECT_DESTINATION) c[t++] = 'F';
 if (flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE) c[t++] = 'B';
 if (flags & FLAGS_REGISTER_NEWNICK) c[t++] = 'T';
 if (flags & FLAGS_SEND_ONLY_IF_DESTINATION_OPER) c[t++] = 'W';
 if (flags & FLAGS_NOT_QUEUE_REQUESTS) c[t++] = 'Z';
 if (t == 1) c[t++] = '-';
 if (mode != 'q') {
     if (send) c[t++] = ']'; else c[t++] = '>';
  }
 c[t] = 0;
}

static void remove_msg(msgclient)
aMsgClient *msgclient;
{
 register aMsgClient **index_p;
 register int t;
 int n,allocate;

 n = tnl_msgclient(msgclient);
 if (msgclient->flags & FLAGS_NAME 
     || !wildcards(msgclient->tonick)) {
     index_p = ToNameList+n; 
     t = toname_index-n;   
     while (t--) {
                   *index_p = index_p[1]; index_p++;
       }
     ToNameList[toname_index] = 0;
     toname_index--;
  } else { 
          index_p = WildCardList+n; 
          t = wildcard_index-n;
          while (t--) {
                        *index_p = index_p[1]; index_p++;
            }
          WildCardList[wildcard_index] = 0;
          wildcard_index--;
     }
 n = fnl_msgclient(msgclient);
 index_p = FromNameList+n;t = fromname_index-n;
 while (t--) {
                *index_p = index_p[1]; index_p++;
   }
 FromNameList[fromname_index] = 0;
 fromname_index--;
 MyFree(msgclient->passwd);
 MyFree(msgclient->fromnick);
 MyFree(msgclient->fromname);
 MyFree(msgclient->fromhost);
 MyFree(msgclient->tonick);
 MyFree(msgclient->toname);
 MyFree(msgclient->tohost);
 MyFree(msgclient->message);
 MyFree((char *)msgclient);
 changes_to_save = 1;
 if (max_fromname - fromname_index > REALLOC_SIZE *2) {
    max_fromname -= REALLOC_SIZE; 
    allocate = max_fromname*sizeof(FromNameList)+1;
    FromNameList = (aMsgClient **) MyRealloc((char *)FromNameList,allocate);
  }
 if (max_wildcards - wildcard_index > REALLOC_SIZE * 2) {
    max_wildcards -= REALLOC_SIZE;
    allocate = max_wildcards*sizeof(WildCardList)+1;
    WildCardList = (aMsgClient **) MyRealloc((char *)WildCardList,allocate);
  }
 if (max_toname - toname_index > REALLOC_SIZE * 2) {
    max_toname -= REALLOC_SIZE;
    allocate = max_toname*sizeof(ToNameList)+1;
    ToNameList = (aMsgClient **) MyRealloc((char *)ToNameList,allocate);
  }
}

static int KeyFlags(sptr, flags) 
aClient *sptr;
long flags;
{
 int t,last, first_tnl, last_tnl, nick_list = 1;
 long clock;
 aMsgClient **index_p,*msgclient; 
 
 t = first_tnl_indexnode(sptr->name);
 last = last_tnl_indexnode(sptr->name);
 first_tnl = first_tnl_indexnode(UserName(sptr));
 last_tnl = last_tnl_indexnode(UserName(sptr));
 index_p = ToNameList;
 time (&clock);
 while (1) {
     while (last && t <= last) {
	    msgclient = index_p[t];
	    if (msgclient->flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS
		&& msgclient->flags & flags
		&& !matches(msgclient->tonick,sptr->name)
		&& !matches(msgclient->toname,UserName(sptr))
		&& !matches(msgclient->tohost,sptr->user->host)) return 1;
            t++;
	}
     if (index_p == ToNameList) {
         if (nick_list) {
             nick_list = 0; t = first_tnl; last = last_tnl;
	  } else {
                   index_p = WildCardList;
                   t = 1; last = wildcard_index;
	      }
      } else return 0;
 }
}

static int set_flags(sptr, string, flags, mode, type)
aClient *sptr;
char *string, mode, *type;
long *flags;
{
 char *c,on,buf[40],cu;
 int op,uf = 0;

 op = IsOperHere(sptr) ? 1:0; 
 for (c = string; *c; c++) {
      if (*c == '+') { 
          on = 1;continue;
       } 
      if (*c == '-') { 
          on = 0;continue;
       } 
      cu = islower(*c)?toupper(*c):*c;
      switch (cu) {
              case 'S': if (on) *flags |= FLAGS_NEWS;
                         else *flags &= ~FLAGS_NEWS;
                        break;             
              case 'R': if (on) *flags |= FLAGS_REPEAT_UNTIL_TIMEOUT;
                         else *flags &= ~FLAGS_REPEAT_UNTIL_TIMEOUT;
                        break;        
              case 'U': if (on) *flags |= FLAGS_NO_NICKCHANGE_SPY;
                         else *flags &= ~FLAGS_NO_NICKCHANGE_SPY;
                        break;             
              case 'V': if (on) *flags |= FLAGS_NICK_AND_WILDCARD_VALID;
                         else *flags &= ~FLAGS_NICK_AND_WILDCARD_VALID;
                        break;             
              case 'I': if (on) *flags |= FLAGS_SEND_ONLY_IF_THIS_SERVER; 
                         else *flags &= ~FLAGS_SEND_ONLY_IF_THIS_SERVER;
                        break;             
              case 'W': if (on) *flags |= FLAGS_SEND_ONLY_IF_DESTINATION_OPER;
                         else *flags &= ~FLAGS_SEND_ONLY_IF_DESTINATION_OPER;
                        break;             
              case 'Q': if (on) *flags |= FLAGS_SEND_ONLY_IF_NICK_NOT_NAME;
                         else *flags &= ~FLAGS_SEND_ONLY_IF_NICK_NOT_NAME;
                        break;             
              case 'E': if (on) *flags |= FLAGS_SEND_ONLY_IF_NOT_EXCEPTION; 
                         else *flags &= ~FLAGS_SEND_ONLY_IF_NOT_EXCEPTION;
                        break;             
              case 'Y': if (on) *flags |= FLAGS_SEND_ONLY_IF_SENDER_ON_IRC;
                         else *flags &= FLAGS_SEND_ONLY_IF_SENDER_ON_IRC;
                        break;             
              case 'N': if (on) *flags |= FLAGS_NOTICE_RECEIVED_MESSAGE;
                         else *flags &= ~FLAGS_NOTICE_RECEIVED_MESSAGE;
                        break;             
              case 'D': if (on) *flags |= FLAGS_DISPLAY_IF_RECEIVED;
                         else *flags &= ~FLAGS_DISPLAY_IF_RECEIVED;
                        break;             
              case 'F': if (on) *flags |= FLAGS_RETURN_CORRECT_DESTINATION;
                         else *flags &= ~FLAGS_RETURN_CORRECT_DESTINATION;
                        break;
              case 'L': if (on) *flags |= FLAGS_DISPLAY_IF_CORRECT_FOUND;
                         else *flags &= ~FLAGS_DISPLAY_IF_CORRECT_FOUND;
                        break;
              case 'X':  if (on) *flags |= FLAGS_DISPLAY_IF_DEST_REGISTER;
                          else *flags &= ~FLAGS_DISPLAY_IF_DEST_REGISTER;
                        break;
              case 'J':  if (on) *flags |= FLAGS_DISPLAY_CHANNEL_DEST_REGISTER;
                          else *flags &= ~FLAGS_DISPLAY_CHANNEL_DEST_REGISTER;
                        break;
              case 'A':  if (on) *flags |= FLAGS_DISPLAY_SERVER_DEST_REGISTER;
                          else *flags &= ~FLAGS_DISPLAY_SERVER_DEST_REGISTER;
                        break;
              case 'C': if (on) *flags |= FLAGS_ALL_NICK_VALID;
                         else *flags &= ~FLAGS_ALL_NICK_VALID;
                        break;
              case 'M': if (on) *flags |= FLAGS_SIGNOFF_REMOVE;
                         else *flags &= ~FLAGS_SIGNOFF_REMOVE;
                        break;
              case 'T': if (KeyFlags(sptr,FLAGS_REGISTER_NEWNICK) 
                            || op || mode == 'd' || !on) {
                            if (on) *flags |= FLAGS_REGISTER_NEWNICK;
                             else *flags &= ~FLAGS_REGISTER_NEWNICK;
                         } else buf[uf++] = cu;
                        break;
              case 'B': if (KeyFlags(sptr,FLAGS_FIND_CORRECT_DEST_SEND_ONCE)
                            || op || mode == 'd' || !on) {
                            if (on) 
                               *flags |= FLAGS_FIND_CORRECT_DEST_SEND_ONCE;
                             else *flags &= ~FLAGS_FIND_CORRECT_DEST_SEND_ONCE;
                         } else buf[uf++] = cu;
                        break;
              case 'K': if (op || mode == 'd' || !on) {
                            if (on) *flags |= FLAGS_KEY_TO_OPEN_OPER_LOCKS;
                             else *flags &= ~FLAGS_KEY_TO_OPEN_OPER_LOCKS;
                         } else buf[uf++] = cu;
                        break;
              case 'O': if (mode == 'd') {
                           if (on) *flags |= FLAGS_WASOPER;
                            else *flags &= ~FLAGS_WASOPER;
       		         } else buf[uf++] = cu;
                         break;
              case 'G': if (mode == 'd') {
                           if (on) *flags |= FLAGS_SERVER_GENERATED_NOTICE;
                             else *flags &= ~FLAGS_SERVER_GENERATED_NOTICE;
		         } else buf[uf++] = cu;
                        break;
              case 'H': if (mode == 'd') {
                           if (on) 
                               *flags |= FLAGS_SERVER_GENERATED_DESTINATION;
                            else *flags &= ~FLAGS_SERVER_GENERATED_DESTINATION;
                          } else buf[uf++] = cu;
                        break;
              case 'Z': if (KeyFlags(sptr,FLAGS_NOT_QUEUE_REQUESTS)
                            || op || mode == 'd' || !on) {
                             if (on) *flags |= FLAGS_NOT_QUEUE_REQUESTS;
			      else *flags &= ~FLAGS_NOT_QUEUE_REQUESTS;
                         } else buf[uf++] = cu;
                         break;  
              default:  buf[uf++] = cu;
        } 
  }
 buf[uf] = 0;
 if (uf) {
     sendto_one(sptr,"NOTICE %s :#?# Unknown flag%s: %s %s",sptr->name,
                uf> 1  ? "s" : "",buf,type);
     return 0;
  }
 if (mode == 's') {
    if (*flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS)
        sendto_one(sptr,"NOTICE %s :### %s", sptr->name,
                   "WARNING: Recipient got keys to unlock the secret portal;");
     else if (*flags & FLAGS_NOT_QUEUE_REQUESTS)
              sendto_one(sptr,"NOTICE %s :### %s",sptr->name,
                         "WARNING: Recipient can't queue requests;");
      else if (*flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE
               && *flags & FLAGS_REPEAT_UNTIL_TIMEOUT
               && send_flag(*flags)) 
               sendto_one(sptr,"NOTICE %s :### %s",sptr->name,
                         "WARNING: Broadcast message in action;");
 }
 return 1;
}

static void split(string, nick, name, host)
char *string, *nick, *name, *host;
{
 char *np = string, *fill;

 *nick = 0; *name = 0; *host = 0;
 fill = nick;
 while (*np) { 
        *fill = *np;
        if (*np == '!') { 
           *fill = 0; fill = name;
	 } else if (*np == '@') { 
                    *fill = 0; fill = host;
 	         } else fill++;
        np++;
   } 
 *fill = 0;       
 if (!*nick) { *nick = '*'; nick[1] = 0; } 
 if (!*name) { *name = '*'; name[1] = 0; } 
 if (!*host) { *host = '*'; host[1] = 0; } 
}

void save_messages()
{
 aMsgClient *msgclient;
 long clock, gflags = 0;
 static long t2 = MAX_DAYS_NO_USE_SPY*3600*24;
 FILE *fp,*fopen();
 int t = 1;
 char *c, mbuf[MSG_LEN], dibuf[40];

 if (!file_inited || !changes_to_save) return;
 time(&clock);
 gflags |= FLAGS_WASOPER; gflags |= FLAGS_NAME;
 gflags |= FLAGS_SERVER_GENERATED_NOTICE;

 while (fromname_index && t <= fromname_index) {
        msgclient = FromNameList[t];
        if (msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER) {
            *mbuf = '\0';
            while (!*mbuf) {
                   strcpy(mbuf, get_msg(msgclient, '2'));
                   if (!*mbuf) update_spymsg(msgclient);
	      }
            if (*get_msg(msgclient, 'n') == '-') {
              if (clock > msgclient->time + t2)
                  msgclient->timeout = 0; 
	    } else {
                     if (clock > msgclient->time + t2 + atoi(mbuf))
                         msgclient->timeout = 0;
                } 
        }       
       if (clock > msgclient->timeout) {
           display_flags(msgclient->flags, dibuf, 'q');
           sprintf(mbuf,"Expired: /Note User -%d %s %s!%s@%s %s", 
                   (int)((msgclient->timeout - msgclient->time)/3600),
                   dibuf, msgclient->tonick, msgclient->toname, 
                   msgclient->tohost, Message(msgclient));
           c = wild_fromnick(msgclient->fromnick, msgclient);
           if (msgclient->timeout
               && !(msgclient->flags & FLAGS_SERVER_GENERATED_NOTICE)
               && !(msgclient->flags & FLAGS_SERVER_GENERATED_DESTINATION))
             new(msgclient->passwd,"SERVER","-","-",
                 c ? c : msgclient->fromnick, msgclient->fromname,
                 local_host(msgclient->fromhost), gflags,
                 24*7*3600+clock, clock, mbuf);
           remove_msg(msgclient); continue;
        }  
       t++;
   }
 changes_to_save = 0;
 fp = fopen(note_save_filename_tmp,"w");
 if (!fp) {
    sendto_ops("Can't open for write: %s", NPATH);
    return;
 }
 w_code(ltoa((long)note_msm),fp);
 w_code(ltoa((long)note_mum),fp);
 w_code(ltoa((long)note_msw),fp);
 w_code(ltoa((long)note_muw),fp);
 w_code(ltoa((long)note_mst),fp);
 w_code(ltoa((long)note_msf),fp);
 t = 1;
 while (fromname_index && t <= fromname_index) {
     msgclient = FromNameList[t];
     w_code(msgclient->passwd,fp),w_code(msgclient->fromnick,fp);
     w_code(msgclient->fromname,fp);w_code(msgclient->fromhost,fp);
     w_code(msgclient->tonick,fp),w_code(msgclient->toname,fp);
     w_code(msgclient->tohost,fp),
     w_code(ltoa(msgclient->flags),fp);
     w_code(ltoa(msgclient->timeout),fp);
     w_code(ltoa(msgclient->time),fp);
     w_code(msgclient->message,fp);
     t++;
  }
 w_code("",fp);
 fclose(fp);
 fp = fopen(note_save_filename_tmp,"r");
 if (!fp || getc(fp) == EOF) {
    sendto_ops("Error writing: %s", note_save_filename_tmp);
    if (fp) fclose(fp); return; 
  }
 fclose (fp);
 unlink(NPATH);link(note_save_filename_tmp,NPATH);
 unlink(note_save_filename_tmp);
 chmod(NPATH, 432);
}

static char *flag_send(aptr, sptr, qptr, nick, msgclient, mode, chn)
aClient *aptr, *sptr, *qptr;
char *nick, mode, *chn;
aMsgClient *msgclient;
{
 int t, t1, exception;
 static char ebuf[BUF_LEN];
 char *c, *message;

 message = Message(msgclient);

 if (MyConnect(sptr)) msgclient->flags |= FLAGS_ON_THIS_SERVER;
     else if (mode != 'e' && mode != 'q') 
              msgclient->flags &= ~FLAGS_ON_THIS_SERVER;
 if (!(msgclient->flags & FLAGS_ON_THIS_SERVER) 
       && msgclient->flags & FLAGS_SEND_ONLY_IF_THIS_SERVER
     || (msgclient->flags & FLAGS_SEND_ONLY_IF_NICK_NOT_NAME 
         && mode == 'v' && StrEq(nick,UserName(sptr)))
     || (!IsOper(sptr) &&
         msgclient->flags & FLAGS_SEND_ONLY_IF_DESTINATION_OPER)
     || (mode == 'v' || mode == 'j' || mode == 'l') && qptr == aptr) 
        return NULLCHAR;

 if (msgclient->flags & FLAGS_SEND_ONLY_IF_NOT_EXCEPTION) {
     c = message;
     for(;;) {
         exception = 0;t = t1 = 0 ;ebuf[0] = 0;
         while (*c != '!') {
                if (!*c || *c == ' ' || t > BUF_LEN) return message;
                ebuf[t++] = *c++;
           }
         if (!*c++) return message;
         t1 += t;ebuf[t] = 0;
         if (!matches(ebuf,nick)) exception = 1;
         t=0;ebuf[0] = 0;            
         while (*c != '@') {
                if (!*c || *c == ' ' || t > BUF_LEN) return message;
                ebuf[t++] = *c++;
           }
         if (!*c++) return message;
         t1 += t;ebuf[t] = 0;
         if (matches(ebuf,UserName(sptr))) exception = 0;
         t=0;ebuf[0] = 0;
         if (*c == ' ') return message;
         while (*c && *c != ' ') {
              if (t > BUF_LEN) return message;
              ebuf[t++] = *c++;
           } 
         if (*c) c++; t1 += t;ebuf[t] = 0; message += t1+2;
         if (*c) message++;
         if (exception && !matches(ebuf,sptr->user->host)) return NULLCHAR;
     }
   }
 return message;
}

static char *check_flags(aptr, sptr, qptr, nick, newnick, qptr_nick,
                         msgclient, repeat, gnew, mode, sptr_chn)
aClient *aptr, *sptr, *qptr;
aMsgClient *msgclient;
char *nick, *newnick, *qptr_nick, mode;
int *repeat, *gnew;
aChannel *sptr_chn;
{
 char *c, mbuf[MSG_LEN], buf[BUF_LEN], ebuf[BUF_LEN], *message, 
      wmode[2], *spy_channel = NULLCHAR, *spy_server = NULLCHAR;
 long clock, gflags;
 int t, t1, t2, last, secret = 1, send = 1, hidden = 0,
     right_tonick = 0, show_channel = 0, sptr_chn_exits = 0;
 aMsgClient *fmsgclient;
 aChannel *chptr;
 Link *link;

 if (mode != 'g') {
    if (!(msgclient->flags & FLAGS_FROM_REG)) qptr = 0;
     else for (qptr = client; qptr; qptr = qptr->next)
            if (qptr->user && 
                (!mycmp(qptr->name,msgclient->fromnick)
                || wild_fromnick(qptr->name, msgclient))
                && !Usermycmp(UserName(qptr),msgclient->fromname)
                && host_check(qptr->user->host,msgclient->fromhost)) 
                break; 
    if (!qptr) msgclient->flags &= ~FLAGS_FROM_REG;
  }
 if (!mycmp(nick, msgclient->tonick)
     || !mycmp(nick, get_msg(msgclient, 'n'))) right_tonick = 1;
 if (!sptr->user->channel && !IsInvisible(sptr)) secret = 0;
 if (secret && !IsInvisible(sptr)) 
  for (link = sptr->user->channel; link; link = link->next)
       if (!SecretChannel(link->value.chptr)) secret = 0;
 if (secret || mode == 'a') hidden = 1; 
 wmode[1] = 0; *repeat = 0; *gnew = 0; 

 if (!send_flag(msgclient->flags)) send = 0; 
 if (mode == 'a' || mode == 'v'
     || mode == 'c' && !wildcards(msgclient->tonick)
     || mode == 'g' && (!wild_fromnick(qptr->name, msgclient)
                        || StrEq(qptr_nick, qptr->name))) {
     msgclient->flags &= ~FLAGS_NEWNICK_DISPLAYED; 
     msgclient->flags &= ~FLAGS_PRIVATE_DISPLAYED;
  }
 message = flag_send(aptr, sptr, qptr, nick, msgclient, mode, sptr_chn);
 if (!message
     || (msgclient->flags & FLAGS_SERVER_GENERATED_NOTICE) && 
        (mode != 'a' && mode != 'v')
     || msgclient->flags & FLAGS_SEND_ONLY_IF_SENDER_ON_IRC && !qptr) { 
    *repeat = 1; return NULLCHAR; 
  }
 if ((!hidden || right_tonick) 
     && msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER) {
     time(&clock);
     mbuf[0] = 0; buf[0] = SPY_CTRLCHAR; buf[1] = 0;
     strcat(mbuf, clean_spychar(nick)); strcat(mbuf, buf);
     strcat(mbuf, clean_spychar(UserName(sptr))); strcat(mbuf, buf);
     strcat(mbuf, clean_spychar(sptr->user->host)); strcat(mbuf, buf);
     strcat(mbuf, clean_spychar(sptr->info)); strcat(mbuf, buf);
     strcat(mbuf, myitoa(clock-msgclient->time)); strcat(mbuf, buf);
     if (*get_msg(msgclient, '2'))
        strcat(mbuf, get_msg(msgclient, '2')); 
      else strcat(mbuf, myitoa(clock-msgclient->time));
     strcat(mbuf, buf);
     t = MSG_LEN - strlen(mbuf) - 10;
     strncat(mbuf, clean_spychar(Message(msgclient)), t);
     strcat(mbuf, "\0");
     DupNewString(msgclient->message, mbuf);
     message = flag_send(aptr, sptr, qptr, nick, msgclient, mode, sptr_chn);
     changes_to_save = 1;
   }
 if (msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER
     && qptr && qptr != sptr) {
     time(&clock);
     t = first_fnl_indexnode(UserName(qptr));
     last = last_fnl_indexnode(UserName(qptr));
     while (last && t <= last) {
            fmsgclient = FromNameList[t];
            if (fmsgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER
                && clock < fmsgclient->timeout
                && fmsgclient != msgclient
                && (!mycmp(mode == 'g' ? qptr_nick : qptr->name, 
                    fmsgclient->fromnick)
                   || wild_fromnick(mode == 'g' ? qptr_nick :qptr->name,
                                    fmsgclient))
                && !Usermycmp(UserName(qptr), fmsgclient->fromname)
                && host_check(qptr->user->host, fmsgclient->fromhost)
                && !matches(fmsgclient->tonick, nick)
                && !matches(fmsgclient->toname, UserName(sptr))
                && !matches(fmsgclient->tohost, sptr->user->host)
                && flag_send(aptr, sptr, qptr, nick, fmsgclient, 
                             mode, sptr_chn)) {
                t1 = wildcards(fmsgclient->tonick) ? 1 : 0;
                t2 = wildcards(msgclient->tonick) ? 1 : 0;
                if (!t1 && t2) goto end_this_flag;
                if (t1 && !t2) { t++; continue; }
                t1 = (fmsgclient->flags & FLAGS_NAME) ? 1 : 0;
                t2 = (msgclient->flags & FLAGS_NAME) ? 1 : 0;
                if (t1 && !t2) goto end_this_flag;
                if (!t1 && t2) { t++; continue; }
                if (fnl_msgclient(fmsgclient) < fnl_msgclient(msgclient))
                    goto end_this_flag;
	      }
            t++;
       }
         mbuf[0] = 0;
         for (link = sptr->user->channel; link; link = link->next) {
              chptr = link->value.chptr;
              if (sptr_chn == chptr) sptr_chn_exits = 1;
              if (IsMember(qptr, chptr)) {
                  msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
                  if (mode != 'j' && mode != 'l') goto end_this_flag;
	       } else if (ShowChannel(qptr, chptr)) show_channel = 1;
              if (ShowChannel(qptr, chptr)) {
                  if (strlen(mbuf)+strlen(chptr->chname) >= MSG_LEN) continue;
                  if (mbuf[0]) strcat(mbuf, " ");
	          strcat(mbuf, chptr->chname);
	       }       
	    }
       for (link = qptr->user->channel; link; link = link->next)
            if (link->value.chptr == sptr_chn) break;
       if (link || mbuf[0] && sptr_chn_exits && !PubChannel(sptr_chn)) 
           mbuf[0] = 0;
        else if (!show_channel) {
               if (msgclient->flags & FLAGS_PRIVATE_DISPLAYED) mbuf[0] = 0;
                else {
                       strcpy(mbuf, "*Private*");
                       msgclient->flags |= FLAGS_PRIVATE_DISPLAYED;
	          }
             } else if (mbuf[0]) msgclient->flags &= ~FLAGS_PRIVATE_DISPLAYED; 

      if (msgclient->flags & FLAGS_DISPLAY_CHANNEL_DEST_REGISTER
          && mode != 'v' && mbuf[0]) spy_channel = mbuf;
      if (msgclient->flags & FLAGS_DISPLAY_SERVER_DEST_REGISTER)
          spy_server = sptr->user->server;
      if (spy_channel || spy_server)
          sprintf(ebuf,"%s%s%s%s%s", spy_channel ? " " : "",
                  spy_channel ? spy_channel : "",
                  spy_server ? " (" : "",
                  spy_server ? spy_server : "",
                  spy_server ? ")" : "");
        else ebuf[0] = 0;
      while (*message == '%') {
            while (*message && *message != ' ') message++;
            if (*message) message++;
	}
      sprintf(buf,"%s@%s",UserName(sptr),sptr->user->host);
      switch (mode) {
        case 'm' :
        case 'l' :
 	case 'j' : if (!(msgclient->flags & FLAGS_NEWNICK_DISPLAYED)
                       && !(only_wildcards(msgclient->tonick)
                       && only_wildcards(msgclient->toname))
                       && !secret && !right_tonick && !spy_channel) {
                       sendto_one(qptr, "NOTICE %s :### %s (%s) %s%s %s",
                                  qptr->name, nick, buf, "signs on",
                                  ebuf, message);
           	       msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
		    } else if (spy_channel && 
                               (!secret || right_tonick)) {
                               sendto_one(qptr,
                                          "NOTICE %s :### %s (%s) is on %s",
                                          qptr->name, nick, buf, spy_channel);
                               msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
			}
                    break;
        case 'v' :
  	case 'a' : if (!hidden || right_tonick) {
                       sendto_one(qptr,
                                  "NOTICE %s :### %s (%s) %s%s %s",
                                  qptr->name, nick, buf, 
                                  "signs on", ebuf, message);
                       msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
		    }
                   break;
	case 'c' : if (!(msgclient->flags & FLAGS_NEWNICK_DISPLAYED)
                       && !(only_wildcards(msgclient->tonick)
                            && only_wildcards(msgclient->toname))
                       && (!secret || right_tonick)) {
                       sendto_one(qptr,
                                  "NOTICE %s :### %s (%s) is %s%s %s",
	     			   qptr->name, nick, buf,
		       		   spy_channel ? "on" : "here",
                                   ebuf,message);
                        msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
                    }
                   break;
        case 's' :
	case 'g' : if ((!secret || right_tonick) &&
                      (!(msgclient->flags & FLAGS_NEWNICK_DISPLAYED)
                        && !(only_wildcards(msgclient->tonick)
                             && only_wildcards(msgclient->toname)))) {
                       sendto_one(qptr,
                                  "NOTICE %s :### %s (%s) is %s%s %s",
	     	                  qptr->name, nick, buf,
                                  spy_channel ? "on" : "on IRC now",
                                  ebuf,message);
                       msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
                    }
                   break;
        case 'q' :
        case 'e' : if (!secret || right_tonick)
                       sendto_one(qptr,
		  	          "NOTICE %s :### %s (%s) %s %s",
				  qptr->name, nick,
			          buf,"signs off", message);
                       break;
	case 'n' : msgclient->flags &= ~FLAGS_NEWNICK_DISPLAYED;
                   if (secret) { 
                       if (right_tonick) 
                           sendto_one(qptr,
                                      "NOTICE %s :### %s (%s) %s %s",
                                      qptr->name, nick, buf,
                                      "signs off", message);
	            } else {
                         if (mycmp(nick, newnick)
                             && !(msgclient->flags & FLAGS_NO_NICKCHANGE_SPY))
                            sendto_one(qptr,
                                       "NOTICE %s :### %s (%s) %s <%s> %s",
                                       qptr->name, nick, buf, 
                                       "changed name to", newnick, message);
                            if (!matches(msgclient->tonick,newnick))
                                msgclient->flags |= FLAGS_NEWNICK_DISPLAYED;
	                 }

      }
        end_this_flag:;
 }

 if (send && !right_tonick && hidden
     && !(msgclient->flags & FLAGS_SERVER_GENERATED_NOTICE)
     && !(msgclient->flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE)) {
     if (mode == 'v') nick = msgclient->tonick;
      else { 
             *repeat = 1; return NULLCHAR;
	   }
 }
 if (mode == 'q' || mode == 'e' || mode == 'n') { 
     *repeat = 1; return NULLCHAR;
  }
 while (mode != 'g' && (msgclient->flags & FLAGS_RETURN_CORRECT_DESTINATION
        || msgclient->flags & FLAGS_DISPLAY_IF_CORRECT_FOUND)) {
        if (*message && matches(message, sptr->info) || 
            hidden && !right_tonick) {
           *repeat = 1; break;
	 }
        time(&clock); 
        sprintf(mbuf,"Search for %s!%s@%s (%s): %s!%s@%s (%s)",
                msgclient->tonick,msgclient->toname,msgclient->tohost,
                *message ? message : "*", nick,
                UserName(sptr),sptr->user->host,sptr->info);
        if (msgclient->flags & FLAGS_DISPLAY_IF_CORRECT_FOUND && qptr) {
            sendto_one(qptr,"NOTICE %s :### %s",qptr->name,mbuf);
            break;
	 }
        t1 = 0; 
        t = first_tnl_indexnode(msgclient->fromname);
        last = last_tnl_indexnode(msgclient->fromname);
        while (last && t <= last) {
              if (ToNameList[t]->timeout > clock &&
                  ToNameList[t]->flags & FLAGS_SERVER_GENERATED_NOTICE &&
                  !Usermycmp(ToNameList[t]->toname, msgclient->fromname) &&
                  !mycmp(ToNameList[t]->tohost,
                         local_host(msgclient->fromhost))) {
                  t1++;
                  if (!mycmp(Message(ToNameList[t]),mbuf)) {
                     t1 = -1; break;
		   }
	       }	  
              t++;
          }  
        if (t1 < 0) break;
        if (t1 > 10) {
            msgclient->timeout = clock-1; 
            break;
	 }
        gflags = 0;
        gflags |= FLAGS_WASOPER;
        gflags |= FLAGS_NAME;
        gflags |= FLAGS_SERVER_GENERATED_NOTICE;
        *gnew = 1;
        c = wild_fromnick(msgclient->fromnick, msgclient);
        new(msgclient->passwd,"SERVER_F","-","-",
            c ? c : msgclient->fromnick, msgclient->fromname,
            local_host(msgclient->fromhost), gflags, 
            24*7*3600+clock, clock, mbuf);
        break;
  }
if (msgclient->flags & FLAGS_REPEAT_UNTIL_TIMEOUT) *repeat = 1;
 
while (send && qptr != sptr &&
        (msgclient->flags & FLAGS_NOTICE_RECEIVED_MESSAGE
         || msgclient->flags & FLAGS_DISPLAY_IF_RECEIVED)) {
        time(&clock);
        if (!right_tonick && hidden && clock-msgclient->time < 3600*24*7)
	   { *repeat = 1; send = 0; break; }
        sprintf(buf,"%s (%s@%s) has received note queued %s before delivery.",
                nick, UserName(sptr), sptr->user->host,
                mytime(msgclient->time));
        if (msgclient->flags & FLAGS_DISPLAY_IF_RECEIVED && 
            qptr && (right_tonick || !hidden)) {
           sendto_one(qptr,"NOTICE %s :### %s", qptr->name, buf);
           break;
         }
       gflags = 0;
       gflags |= FLAGS_WASOPER;
       gflags |= FLAGS_SERVER_GENERATED_NOTICE;
       time(&clock);*gnew = 1;
       c = wild_fromnick(msgclient->fromnick, msgclient);
       new(msgclient->passwd,"SERVER","-","-",
           c ? c : msgclient->fromnick, msgclient->fromname,
           local_host(msgclient->fromhost), gflags,note_mst*24*3600+clock,
           clock,buf);
       break;
    }
 while (send && msgclient->flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE) {
        if (mode == 'g') {
            *repeat = 1; send = 0; break;
	  }
        t = first_tnl_indexnode(UserName(sptr));
        last = last_tnl_indexnode(UserName(sptr));
        while (last && t <= last) {
             if (ToNameList[t]->flags & FLAGS_SERVER_GENERATED_DESTINATION
                 && (!mycmp(ToNameList[t]->fromnick,msgclient->fromnick)
                     || wild_fromnick(ToNameList[t]->fromnick, msgclient))
                 && !Usermycmp(ToNameList[t]->fromname,msgclient->fromname)
                 && host_check(ToNameList[t]->fromhost,msgclient->fromhost)
                 && (!(msgclient->flags & FLAGS_REGISTER_NEWNICK) 
                     || !mycmp(ToNameList[t]->tonick,nick))
                 && !Usermycmp(ToNameList[t]->toname,UserName(sptr))
                 && host_check(ToNameList[t]->tohost,sptr->user->host)) {
                 send = 0; break;
             }
             t++;
	  }
        if (!send) break;
        gflags = 0;
        gflags |= FLAGS_NAME;
        gflags |= FLAGS_REPEAT_UNTIL_TIMEOUT;
        gflags |= FLAGS_SERVER_GENERATED_DESTINATION;
        if (msgclient->flags & FLAGS_ALL_NICK_VALID) 
         gflags |= FLAGS_ALL_NICK_VALID;
        if (msgclient->flags & FLAGS_NICK_AND_WILDCARD_VALID) 
         gflags |= FLAGS_NICK_AND_WILDCARD_VALID;
        if (msgclient->flags & FLAGS_WASOPER) gflags |= FLAGS_WASOPER;
        time(&clock); *gnew = 1;
        new(msgclient->passwd,msgclient->fromnick, msgclient->fromname,
            msgclient->fromhost,nick,UserName(sptr),
            sptr->user->host,gflags,msgclient->timeout,clock,"");
        break;
   }
  if (send) return message; 
   else return NULLCHAR;
}

static void check_messages(aptr, sptr, info, mode)
aClient *sptr, *aptr; /* aptr = who activated */
void *info;
char mode;
{
 aMsgClient *msgclient, **index_p;
 char *qptr_nick, dibuf[40], *newnick, *message, *tonick;
 int last, first_tnl = 0, last_tnl = 0, first_tnil, last_tnil, nick_list = 1,
     number_matched = 0, t, repeat, gnew;
 long clock,flags;
 aClient *qptr = sptr; /* qptr points to active person queued a request */
 aChannel *sptr_chn = 0;

 if (!file_inited) init_messages();
 if (mode == 'm' || mode == 'j' || mode == 'l') {
     sptr_chn = (aChannel *)info; qptr_nick = sptr->name;
  } else qptr_nick = (char *)info;
 tonick = qptr_nick;
 if (!sptr->user || !*tonick) return;
 if (mode == 'a' && StrEq(sptr->info,"IRCnote") /* Removed in next release */
     && StrEq(sptr->name, sptr->info)) {
         sendto_one(sptr,":%s NOTICE %s : <%s> %s (%d) %s",
                    me.name, tonick, me.name, VERSION, number_fromname(),
                    StrEq(ptr,"IRCnote") ? "" : (!*ptr ? "-" : ptr));
      }
 time(&clock);
 if (!fromname_index) return;
 if (mode != 'j' && mode != 'l') {
     t = first_fnl_indexnode(UserName(sptr));
     last = last_fnl_indexnode(UserName(sptr));
     while (last && t <= last) {
           msgclient = FromNameList[t];
           if (!host_check(msgclient->fromhost, sptr->user->host)) {
               t++; continue;
            }
           if (!mycmp(qptr_nick, msgclient->fromnick)
               || wild_fromnick(qptr_nick, msgclient)) {
               if (msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER)
                   update_spymsg(msgclient);
               if ((mode == 'e' || mode == 'q')
                   && msgclient->flags & FLAGS_SIGNOFF_REMOVE)
                   msgclient->timeout = 0;
	    }
           msgclient->flags |= FLAGS_FROM_REG;
           t++; 
        }
   }
 if (clock > old_clock+note_msf) {
    save_messages(); old_clock = clock;
  }
 if (mode == 'v') {
     nick_list = 0; /* Rest done with A flag */
  }
 if (mode == 'n') {
     newnick = tonick;tonick = sptr->name;
   }
 if (mode != 'g') {
     first_tnl = first_tnl_indexnode(UserName(sptr));
     last_tnl = last_tnl_indexnode(UserName(sptr));
     first_tnil = first_tnl_indexnode(tonick);
     last_tnil = last_tnl_indexnode(tonick);
  }
 if (mode == 's' || mode == 'g') {
     t = first_fnl_indexnode(UserName(sptr));
     last = last_fnl_indexnode(UserName(sptr));
     index_p = FromNameList;
     sptr = client; /* Notice new sptr */
     while (sptr && (!sptr->user || !*sptr->name)) sptr = sptr->next;
     if (!sptr) return;
     tonick = sptr->name;
  } else {
           if (nick_list) {
             t = first_tnil; last = last_tnil;
	    } else {
                     t = first_tnl; last = last_tnl;
		}
           index_p = ToNameList;
      }
 while(1) {
    while (last && t <= last) {
           msgclient = index_p[t];
           if (msgclient->timeout < clock
               || index_p == WildCardList  
                  && (send_flag(msgclient->flags) 
                      || msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER)
                  && ((msgclient->flags & FLAGS_WASOPER 
                       && !valid_elements(msgclient->tohost))
                       || !(msgclient->flags & FLAGS_WASOPER)
                          && matches(local_host(sptr->user->host), 
                                     msgclient->tohost))) {
              t++; continue;
	    }
           gnew = 0; repeat=1;
           if (!(msgclient->flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS)
               && !(msgclient->flags & FLAGS_SERVER_GENERATED_DESTINATION)
               && (index_p != ToNameList 
                   || (!nick_list && msgclient->flags & FLAGS_NAME)
                   || (nick_list && !(msgclient->flags & FLAGS_NAME)))
               && !matches(msgclient->tonick, tonick)
               && !matches(msgclient->toname, UserName(sptr))
               && !matches(msgclient->tohost, sptr->user->host)
               && (mode != 's' && mode != 'g'
                   || (wild_fromnick(qptr_nick, msgclient)
                       || !mycmp(qptr_nick, msgclient->fromnick))
                       && host_check(msgclient->fromhost, 
                                     qptr->user->host))) {
               message = check_flags(aptr, sptr, qptr, tonick, newnick,
                                     qptr_nick, msgclient, &repeat, 
                                     &gnew, mode, sptr_chn);
               if (message) {
                   flags = msgclient->flags;
                   display_flags(flags, dibuf, '-');
                   if (flags & FLAGS_SERVER_GENERATED_NOTICE)
                       sendto_one(sptr,"NOTICE %s :/%s/ %s",
                                  tonick, mytime(msgclient->time), message);
                   else sendto_one(sptr,
                                   "NOTICE %s :Note from %s!%s@%s /%s/ %s %s",
                                    tonick, msgclient->fromnick, 
                                    msgclient->fromname, msgclient->fromhost, 
                                    mytime(msgclient->time), dibuf, message);
       	        }
               if (!(msgclient->flags & FLAGS_NAME)) number_matched++;
	     }
           if (!repeat) msgclient->timeout = 0;
           if (gnew) {
               if (mode != 'g') {
                  first_tnl = first_tnl_indexnode(UserName(sptr));
                  last_tnl = last_tnl_indexnode(UserName(sptr));
                  first_tnil = first_tnl_indexnode(tonick);
                  last_tnil = last_tnl_indexnode(tonick);
		}
               if (mode == 's' || mode == 'g') {
                  t = fnl_msgclient(msgclient);
                  last = last_fnl_indexnode(UserName(qptr));
		} else {
                         if (index_p == ToNameList) {
                             t = tnl_msgclient(msgclient);
                             if (nick_list) last = last_tnil; 
                              else last = last_tnl;
      		          }
		    }
            } 
           if (repeat && (mode == 's' || mode =='g')) {
               sptr = sptr->next;
               while (sptr && (!sptr->user || !*sptr->name)) sptr = sptr->next;
                if (!sptr || number_matched) {
                    number_matched = 0; sptr = client; 
                    while (sptr && (!sptr->user || !*sptr->name))
                           sptr = sptr->next;
                    if (!sptr) return;
                    tonick = sptr->name; t++; continue;
		 }
               tonick = sptr->name;
            } else t++;
	 }
     if (mode == 's' || mode == 'g') return;
     if (index_p == ToNameList) {
         if (nick_list) {
             if (mode == 'a' || mode == 'q') break;
             nick_list = 0; t = first_tnl; last = last_tnl;
	  } else {
                  index_p = WildCardList;
                  t = 1; last = wildcard_index;
	     }
      } else {
               if (mode == 'n') {
                   mode = 'c'; tonick = newnick;
                   first_tnil = first_tnl_indexnode(tonick);
                   last_tnil = last_tnl_indexnode(tonick);
                   t = first_tnil; last = last_tnil;
                   nick_list = 1; index_p = ToNameList;
	        } else break;
	 }
  }
  if (mode == 'c' || mode == 'a') 
     check_messages(sptr, sptr, tonick, 'g');
}

static int check_lastclient(sptr, mode, clock, sptr_chn)
aClient *sptr;
char mode;
long clock;
aChannel *sptr_chn;
{
  static aClient *client_list[LAST_CLIENTS + 1];
  static long max_last_client = -1, time_list[LAST_CLIENTS + 1], reset = 0;
  static char mode_list[LAST_CLIENTS + 1];
  static aChannel *chn_list[LAST_CLIENTS + 1];
  register aClient *sptr_exist, *last_client;
  register int t = 0, new = -1, multi_buffered = 0, ret = 1;
  register long longest_time = 0;
  register char b_mode;
  static long last_check_time = 0;

 if (!reset) { 
    for (t = 0; t < LAST_CLIENTS; t++) client_list[t] = 0;
    reset = 1;
  }
 if (mode == 'j' || mode == 'e' || mode == 'l')
     for (t = 0; t <= max_last_client; t++) {
         if (client_list[t] != sptr) continue;
         b_mode = mode_list[t];
         if (mode == 'e') {
            if (b_mode == 'v' && ret) {
                ret = 0; check_messages(sptr, sptr, sptr->name, 'q');
	     }
            client_list[t] = 0;
	  } else if (b_mode == 'j' || b_mode == 'l') {
                     multi_buffered = 1;
                     time_list[t] = clock;
                     mode_list[t] = mode;
		     chn_list[t] = 0;
                     break;
	          }
       }
  if (!multi_buffered && 
      (mode == 'a' || mode == 'j' || mode == 'l' || mode == 'o'))
      for (t = 0; t < LAST_CLIENTS; t++) {
           if (!client_list[t]) { new = t; break; }
           if (time_list[t] > longest_time) {
               longest_time = time_list[t];
               new = t;
	    }
       }
 if (mode != 'a' && mode != 'j' && mode != 'l' || last_check_time != clock) {
     last_check_time = clock;
     for (t = 0; t <= max_last_client; t++)
         if (client_list[t] && 
             (clock - time_list[t] > SECONDS_WAITFOR_MODE 
             || (client_list[t] == sptr && mode != 'j' 
                 && mode != 'l' && mode != 'o') 
             || t == new)) {
             last_client = client_list[t];
             for (sptr_exist=client;sptr_exist;sptr_exist = sptr_exist->next)
             if (sptr_exist == last_client) break;
             if (sptr_exist) {
                if (mode_list[t] == 'j' || mode_list[t] == 'l')
                 check_messages(sptr ? sptr : client_list[t], client_list[t], 
                                chn_list[t], mode_list[t]);
                 else check_messages(sptr ? sptr : client_list[t], 
                                     client_list[t], client_list[t]->name, 
                                     mode_list[t]);
              }
             client_list[t] = 0;
             if (max_last_client == t)
              while (max_last_client > -1 && !client_list[max_last_client]) 
                max_last_client--;
         }
   }
 if (new != -1) {
    client_list[new] = sptr; 
    time_list[new] = clock;
    chn_list[new] = sptr_chn;
    if (mode == 'a') mode_list[new] = 'v';
     else mode_list[new] = mode; 
    if (new > max_last_client) max_last_client = new;
  }
 return ret;
}

void check_command(info, command, par1, par2, par3)
void *info;
char *command, *par1, *par2, *par3;
{
 char *arg, *c, mode, nick[BUF_LEN]; 
 static char last = 0, last_command[BUF_LEN], last_nick[BUF_LEN], 
        last_chn[BUF_LEN], last_mode = 0, last_modes[BUF_LEN];
 int from_secret = 0, on = -1;
 long *delay, clock, last_call = 0, source;
 Link *link;
 aChannel *chptr = 0;
 aClient *sptr;

 time (&clock);
 if (!last) {
     last_command[0] = 0; last_nick[0] = 0; 
     last_modes[0] = 0, last_chn[0] = 0; last = 1;
  }
 if (!command) {
        delay = (long *) info;
        if (*delay > SECONDS_WAITFOR_MODE) *delay = SECONDS_WAITFOR_MODE;
        if (clock - last_call < SECONDS_WAITFOR_MODE) return;
        last_call = clock;
        check_lastclient((aClient *)0, (char)'-', clock, (aChannel *)0);
        return;
  } 
 source = (long) info;
 arg = split_string(command, 2, 1);
 if (StrEq(arg, "USER")) mode = 'a'; else
 if (StrEq(arg, "NICK")) mode = 'n'; else
 if (StrEq(arg, "JOIN")) mode = 'j'; else
 if (StrEq(arg, "MODE")) mode = 'm'; else
 if (StrEq(arg, "QUIT")) mode = 'e'; else
 if (StrEq(arg, "PART") || StrEq(arg, "KICK")) mode = 'l'; else 
 if (StrEq(arg, "KILL")) {
     par1 = par2; /* Who kills before who's killed here... */
     mode = 'e';
 } else return;
 strcpy(nick, par1);
 if ((c = (char *)index(nick, '!')) != NULL) *c = '\0';
 if ((c = (char *)index(nick, '[')) != NULL) *c = '\0';
 if (mode == 'm' && (c = (char *)index(command, '+')) != NULL) par3 = c;
 if (mode == 'm' && (c = (char *)index(command, '-')) != NULL) par3 = c;
 if (mode == 'm' && (c = (char *)index(command, 'c')) != NULL) return;
 if (mode == 'j' && (c = (char *)index(command, '0')) != NULL) par2 = c+1;

 if (last_mode == mode && StrEq(last_nick, nick)) {
     if (mode == 'm') {
         if (StrEq(last_modes, par3)) return;
      }  else if (mode == 'j' || mode == 'l') {
                  if (StrEq(last_chn, par2)) return;
               } else return;
  }
 if (mode == 'm') strncpyzt(last_modes, par3, BUF_LEN-1);
 if (mode == 'j' || mode == 'l') strncpyzt(last_chn, par2, BUF_LEN-1);
 strncpyzt(last_command, command, BUF_LEN-1); 
 strncpyzt(last_nick, nick, BUF_LEN-1); 
 last_mode = mode;

 if (mode == 'm' && *par3 == '+' && par3[1] == 'o') mode = 'o';
 sptr = find_person(nick, (aClient *)NULL);
 if (!sptr) for (sptr = client; sptr; sptr = sptr->next)
                 if (StrEq(sptr->name, nick)) break;
 if (!sptr || !sptr->user) return;
 if (mode != 'j' && mode != 'l' && mode != 'm') {
     if (check_lastclient(sptr, mode, clock, chptr) && mode != 'o')
         check_messages(sptr, sptr, mode == 'n' ? par2 : sptr->name, mode);
     return;
  }
 for (link = sptr->user->channel; link; link = link->next)
      if (link->value.chptr && 
          StrEq(par2, link->value.chptr->chname)) break;
 if (link) chptr = link->value.chptr;
 for (link = sptr->user->channel; link; link = link->next)
      if (link->value.chptr != chptr && PubChannel(link->value.chptr)) break;
 if (chptr && link && !PubChannel(chptr)) return;  
 if (mode != 'm') {
     check_lastclient(sptr, mode, clock, chptr);
     return;
  }
 for (c = par3; *c; c++) {
      if (*c == '+') { 
          on = 1;continue;
       } 
      if (*c == '-') { 
          on = 0;continue;
       } 
      if (!on && (!chptr && *c == 'i' || *c == 'p' || *c == 's')
          && (!chptr || PubChannel(chptr))) from_secret = 1;
   }
 if (!from_secret) return;
 if (chptr)
     for (link = chptr->members; link; link = link->next) {
          check_lastclient(link->value.cptr, mode, clock, chptr);
          check_messages(link->value.cptr, link->value.cptr, chptr, mode);
      }
  else {
         check_lastclient(sptr, mode, clock, chptr);
         check_messages(sptr, sptr, chptr, mode);
    }
}

static void msg_remove(sptr, passwd, flag_s, id_s, name, time_s)
aClient *sptr;
char *passwd, *flag_s, *id_s, *name, *time_s;
{
 aMsgClient *msgclient;
 int removed = 0, t, last, id;
 long clock, flags = 0, time_l;
 char dibuf[40], tonick[BUF_LEN], toname[BUF_LEN], tohost[BUF_LEN];

 if (!set_flags(sptr,flag_s, &flags,'d',"")) return;
 if (!*time_s) time_l = 0; 
  else { 
        time_l = set_date(sptr,time_s);
        if (time_l < 0) return;
    }
 split(name, tonick, toname, tohost);
 if (id_s) id = atoi(id_s); else id = 0;
 t = first_fnl_indexnode(UserName(sptr));
 last = last_fnl_indexnode(UserName(sptr));
 time (&clock);
 while (last && t <= last) {
       msgclient = FromNameList[t]; flags = msgclient->flags;
        if (clock > msgclient->timeout) { t++; continue; }
        set_flags(sptr, flag_s, &flags, 'd',"");
        if (local_check(sptr,msgclient,passwd,flags,
                        tonick,toname,tohost,time_l,id)) {
            display_flags(msgclient->flags, dibuf, '-'),
            sendto_one(sptr,"NOTICE %s :### Removed -> %s %s (%s@%s)",
                       sptr->name,dibuf,msgclient->tonick,
                       msgclient->toname,msgclient->tohost);
            remove_msg(msgclient); last--; removed++;

        } else t++;
   }
 if (!removed) 
  sendto_one(sptr,"NOTICE %s :#?# No such request(s) found", sptr->name);
}

static void msg_save(sptr)
aClient *sptr;
{
 if (!changes_to_save) {
     sendto_one(sptr,"NOTICE %s :### No changes to save",sptr->name);
     return;
 }
 if (IsOperHere(sptr)) {
    save_messages();
    sendto_one(sptr,"NOTICE %s :### Requests are now saved",sptr->name);
  } else
      sendto_one(sptr,"NOTICE %s :### Oops! All requests lost...",sptr->name);
}

static void setvar(sptr,msg,l,value)
aClient *sptr;
int *msg,l;
char *value;
{
 int max;
 static char *message[] = {
             "Max server messages:",
             "I don't think that this is a good idea...",
             "Max server messages are set to:",
             "Max server messages with wildcards:",
             "Too many wildcards makes life too hard...",
             "Max server messages with wildcards are set to:",
             "Max user messages:",
             "Too cheeky fingers on keyboard error...",
             "Max user messages are set to:",
             "Max user messages with wildcards:",
             "Give me $$$, and I may fix your problem...",
             "Max user messages with wildcards are set to:",
             "Max server days:",
             "Can't remember that long time...",
             "Max server days are set to:",
             "Note save frequency:",
             "Save frequency may not be like that...",
             "Note save frequency is set to:" 
          };

 if (*value) {
    max = atoi(value);
    if (!IsOperHere(sptr))
        sendto_one(sptr,"NOTICE %s :### %s",sptr->name,message[l+1]);
     else { 
           if (!max && (msg == &note_mst || msg == &note_msf)) max = 1;
           *msg = max;if (msg == &note_msf) *msg *= 60;
           sendto_one(sptr,"NOTICE %s :### %s %d",sptr->name,message[l+2],max);
           changes_to_save = 1;
       }
 } else {
         max = *msg; if (msg == &note_msf) max /= 60;
         sendto_one(sptr,"NOTICE %s :### %s %d",sptr->name,message[l],max);
     }
}

static void msg_stats(sptr, arg, value)
aClient *sptr;
char *arg, *value;
{
 long clock;
 char buf[BUF_LEN], *fromhost = NULLCHAR, *fromnick, *fromname;
 int tonick_wildcards = 0,toname_wildcards = 0,tohost_wildcards = 0,any = 0,
     nicks = 0,names = 0,hosts = 0,t = 1,last = fromname_index,flag_notice = 0,
     flag_destination = 0, all_wildcards = 0;
 aMsgClient *msgclient;

 if (*arg) {
     if (!mycmp(arg,"MSM")) setvar(sptr,&note_msm,0,value); else 
     if (!mycmp(arg,"MSW")) setvar(sptr,&note_msw,3,value); else
     if (!mycmp(arg,"MUM")) setvar(sptr,&note_mum,6,value); else
     if (!mycmp(arg,"MUW")) setvar(sptr,&note_muw,9,value); else
     if (!mycmp(arg,"MST")) setvar(sptr,&note_mst,12,value); else
     if (!mycmp(arg,"MSF")) setvar(sptr,&note_msf,15,value); else
     if (MyEq(arg,"USED")) {
         time(&clock);
         while (last && t <= last) {
                msgclient = FromNameList[t];
                if (clock > msgclient->timeout) { t++; continue; }
                any++;
                if (msgclient->flags & FLAGS_SERVER_GENERATED_DESTINATION)
                    flag_destination++; else
                if (msgclient->flags & FLAGS_SERVER_GENERATED_NOTICE)
                    flag_notice++; else
                if (IsOperHere(sptr) || Key(sptr))
                    if (!fromhost || 
                        !host_check(msgclient->fromhost,fromhost)) {
                        nicks++;names++;hosts++;
                        fromhost = msgclient->fromhost;
                        fromname = msgclient->fromname;
                        fromnick = msgclient->fromnick;
                     } else if (Usermycmp(msgclient->fromname,fromname)) {
                                nicks++;names++;
                                fromname = msgclient->fromname;
                                fromnick = msgclient->fromnick;
            	             } else if (mycmp(msgclient->fromnick,fromnick)) {
                                        nicks++;
                                        fromnick = msgclient->fromnick;
	  	                     } 
                if (wildcards(msgclient->tonick) && 
                    wildcards(msgclient->toname)) all_wildcards++;
                if (wildcards(msgclient->tonick))
                    tonick_wildcards++;
                if (wildcards(msgclient->toname))
                    toname_wildcards++;
                if (wildcards(msgclient->tohost))
                    tohost_wildcards++;
                t++;
	    }
	    if (!any) 
             sendto_one(sptr,"NOTICE %s :#?# No request(s) found",sptr->name);
                else {
                     if (IsOperHere(sptr) || Key(sptr)) {
                         sprintf(buf,"%s%d %s%d %s%d %s (%s%d %s%d %s%d %s%d)",
                                 "Nicks:",nicks,"Names:",names,
                                 "Hosts:",hosts,"W.cards",
                                 "Nicks:",tonick_wildcards,
                                 "Names:",toname_wildcards,
                                 "Hosts:",tohost_wildcards,
                                 "All:",all_wildcards);
                         sendto_one(sptr,"NOTICE %s :### %s",sptr->name,buf);
		      }
                      sprintf(buf,"%s %s%d / %s%d",
                              "Server generated",
                              "G-notice: ",flag_notice,
                              "H-header: ",flag_destination);
                      sendto_one(sptr,"NOTICE %s :### %s",sptr->name,buf);
		 }
     } else if (!mycmp(arg,"RESET")) {
                if (!IsOperHere(sptr)) 
                    sendto_one(sptr,"NOTICE %s :### %s",sptr->name,
                               "Wrong button - try another next time...");
                 else {
                       note_mst = NOTE_MAXSERVER_TIME,
                       note_mum = NOTE_MAXUSER_MESSAGES,
                       note_msm = NOTE_MAXSERVER_MESSAGES,
                       note_msw = NOTE_MAXSERVER_WILDCARDS,
                       note_muw = NOTE_MAXUSER_WILDCARDS;
                       note_msf = NOTE_SAVE_FREQUENCY*60;
                       sendto_one(sptr,"NOTICE %s :### %s",
                                  sptr->name,"Stats have been reset");
                       changes_to_save = 1;
		   }
	    }
  } else {
      t = number_fromname();
      sprintf(buf,"%s%d /%s%d /%s%d /%s%d /%s%d /%s%d /%s%d",
              "QUEUE:",t,
              "MSM:",note_msm,
              "MSW:",note_msw,
              "MUM:",note_mum,
              "MUW:",note_muw,
              "MST:",note_mst,
              "MSF:",note_msf/60);
        sendto_one(sptr,"NOTICE %s :### %s",sptr->name,buf);
    }
}

static int msg_send(sptr, silent, passwd, flag_s, timeout_s, name, message)
aClient *sptr;
int silent;
char *passwd, *flag_s, *timeout_s, *name, *message;
{
 aMsgClient **index_p, *msgclient;
 int sent_wild = 0, sent = 0, nick_list = 1,
     first_tnl, last_tnl, t, first, last, join = 0;
 long clock, timeout, flags = 0;
 char buf[BUF_LEN], dibuf[40], *empty_char = "", tonick[BUF_LEN], 
      toname[BUF_LEN], tohost[BUF_LEN], *msg;

 time (&clock);
 t = first_tnl_indexnode(sptr->name);
 last = last_tnl_indexnode(sptr->name);
 first_tnl = first_tnl_indexnode(UserName(sptr));
 last_tnl = last_tnl_indexnode(UserName(sptr));
 index_p = ToNameList;
 time (&clock);
 while (1) {
     while (last && t <= last) {
  	    msgclient = index_p[t];
	    if (clock > msgclient->timeout) { t++; continue; }
            msg = flag_send((aClient *)0, sptr, (aClient *)0 , sptr->name, 
                  msgclient, '-', NULLCHAR);
            if (msg && msgclient->flags & FLAGS_NOT_QUEUE_REQUESTS
	        && !(msgclient->flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS)
 	        && !matches(msgclient->tonick,sptr->name) 
 	        && !matches(msgclient->toname,UserName(sptr))
	        && !matches(msgclient->tohost,sptr->user->host)) {
		sendto_one(sptr,"NOTICE %s :### %s (%s@%s) %s %s",sptr->name,
			   msgclient->fromnick,msgclient->fromname,
			   msgclient->fromhost,
                           "doesn't allow you to queue requests:", msg);
		return -1;
	    }
            t++;
	}
     if (index_p == ToNameList) {
         if (nick_list) {
             nick_list = 0; t = first_tnl; last = last_tnl;
          } else {
                   index_p = WildCardList;
                   t = 1; last = wildcard_index;
              }

      } else break;
 }
 if (number_fromname() >= note_msm 
     && !IsOperHere(sptr) && !Key(sptr)) {
     if (!note_msm || !note_mum)
         sendto_one(sptr,
                    "NOTICE %s :#?# The notesystem is closed for no-operators",
                    sptr->name);
      else sendto_one(sptr,"NOTICE %s :#?# No more than %d request%s %s",
                      sptr->name, note_msm, note_msm < 2 ? "" : "s",
                      "allowed in the server");
     return -1;
  }
 if (clock > old_clock+note_msf) {
    save_messages();old_clock = clock;
  }
 if (!set_flags(sptr,flag_s,&flags,'s',"")) return -1;
 split(name, tonick, toname, tohost);
 if (IsOper(sptr)) flags |= FLAGS_WASOPER;
 if (*timeout_s == '+') timeout = atoi(timeout_s + 1) * 24;
  else if (*timeout_s == '-') timeout = atoi(timeout_s + 1);
 if (timeout > note_mst*24 && !(flags & FLAGS_WASOPER) && !Key(sptr)) {
    sendto_one(sptr,"NOTICE %s :#?# Max time allowed is %d day%s",
               sptr->name,note_mst,note_mst > 1 ? "s" : "");
    return -1;
  }
 if (!message) {
    if (!send_flag(flags)) message = empty_char; 
     else {
           sendto_one(sptr,"NOTICE %s :#?# No message specified",sptr->name);
           return -1;
       }
  }
 if (*toname == '#') {
     sendto_one(sptr,"NOTICE %s :#?# Please skip # character in username",
                sptr->name);
     return -1;
  }
 first = first_fnl_indexnode(UserName(sptr));
 last = last_fnl_indexnode(UserName(sptr));
 t = first;
 while (last && t <= last) {
        msgclient = FromNameList[t];
        if (clock > msgclient->timeout) { t++; continue; }
        if (!mycmp(sptr->name, msgclient->fromnick)
            && !Usermycmp(UserName(sptr), msgclient->fromname)
            && host_check(sptr->user->host, msgclient->fromhost)
            && StrEq(msgclient->tonick, tonick)
            && StrEq(msgclient->toname, toname)
            && StrEq(msgclient->tohost, tohost)
            && StrEq(msgclient->passwd, passwd)
            && StrEq(Message(msgclient), clean_spychar(message))
            && msgclient->flags == (msgclient->flags | flags)) {
            msgclient->timeout = timeout*3600+clock;
            join = 1;
	  }
        t++;
    }
  if (!join && !(flags & FLAGS_WASOPER) && !Key(sptr)) {
     time(&clock); t = first;
     while (last && t <= last) {
         if (!Usermycmp(UserName(sptr),FromNameList[t]->fromname)) {
             if (host_check(sptr->user->host,FromNameList[t]->fromhost)) {
                 sent++;
                 if (wildcards(FromNameList[t]->tonick)
                     && wildcards(FromNameList[t]->toname))
                    sent_wild++;  
              }
                 
          }
         t++;
       }
     if (sent >= note_mum) {
        sendto_one(sptr,"NOTICE %s :#?# No more than %d request%s %s",
                   sptr->name,note_mum,note_mum < 2?"":"s",
                   "for each user allowed in the server");
        return -1;
      }
     while (wildcards(tonick) && wildcards(toname)) {
            if (!note_msw || !note_muw)
                sendto_one(sptr,
                          "NOTICE %s :#?# No-operators are not allowed %s",
                          sptr->name,
                          "to specify nick and username with wildcards");
            else if (wildcard_index >= note_msw) 
                     sendto_one(sptr,"NOTICE %s :#?# No more than %d req. %s",
                                sptr->name, note_msw,
                                "with nick and username w.cards allowed.");
            else if (sent_wild >= note_muw) 
                     sendto_one(sptr,"NOTICE %s :#?# No more than %d %s %s",
                                sptr->name, note_muw, 
                                note_muw < 2 ? "request ":" requests",
                                "with nick and username w.cards allowed.");
          else break;
          return -1;
     }
   }
 while ((send_flag(flags) || flags & FLAGS_DISPLAY_IF_DEST_REGISTER) &&
       wildcards(tonick) && wildcards(toname)) { 
       if (flags & FLAGS_WASOPER && !valid_elements(tohost))
          sendto_one(sptr, 
                     "NOTICE %s :#?# This matches more than one country.",
                     sptr->name);
        else if (!(flags & FLAGS_WASOPER) &&
                 matches(local_host(sptr->user->host), tohost))
                 sendto_one(sptr, "NOTICE %s :#?# %s must be a local host.",
                            sptr->name, local_host(sptr->user->host));
          else break; 
       return -1;
  }
 if (!join) {
     time(&clock);
     flags |= FLAGS_FROM_REG;
     msgclient = new(passwd,sptr->name, UserName(sptr),  
                     sptr->user->host, tonick, toname, tohost, flags, 
                     timeout*3600+clock, clock, clean_spychar(message));
     if (flags & FLAGS_DISPLAY_IF_DEST_REGISTER) update_spymsg(msgclient);
  }
 display_flags(flags, dibuf, '-');
 sprintf(buf, "%s %s %s!%s@%s for %s",
         join ? "Joined..." : "Queued...",dibuf, 
         tonick, toname, tohost, relative_time(timeout*3600));
 if (!silent) {
    sendto_one(sptr,"NOTICE %s :### %s",sptr->name, buf);
    if (send_flag(flags)) check_messages(sptr, sptr, sptr->name, 's');
  }
 if (join) return 0; else return 1;
}

static void msg_news(sptr, silent, passwd, flag_s, timeout_s, name, message)
aClient *sptr;
int silent;
char *passwd, *flag_s, *timeout_s, *name, *message;
{
 aMsgClient *msgclient;
 long clock;
 int joined = 0, queued = 0, ret, t = 1, msg_len;
 char *c, tonick[BUF_LEN], toname[BUF_LEN], tohost[BUF_LEN], 
      anyname[BUF_LEN], buf[MSG_LEN];

 split(name, tonick, toname, tohost);
 if (MyEq("ADMIN.", tonick) && !IsOper(sptr) && !KeyFlags(sptr, FLAGS_NEWS)) {
     sendto_one(sptr,
                "NOTICE %s :#?# No privileges for admin group.", 
                sptr->name);
     return;
  }
 time (&clock);
 sprintf(buf, "[News:%s] ", tonick); msg_len = MSG_LEN-strlen(buf)-1;
 strncat(buf, message, msg_len); strcat(flag_s, "-RS");
 while (fromname_index && t <= fromname_index) {
        msgclient = FromNameList[t];
        if (!Usermycmp(UserName(sptr), msgclient->fromname)
            && (!mycmp(sptr->name, msgclient->fromnick)
                || wild_fromnick(sptr->name, msgclient)
            &&  host_check(sptr->user->host,msgclient->fromhost))) {
	    t++; continue;
	 }
        if (clock < msgclient->timeout
            && !(msgclient->flags & FLAGS_SERVER_GENERATED_NOTICE)
            && !(msgclient->flags & FLAGS_SERVER_GENERATED_DESTINATION)  
            && (msgclient->flags & FLAGS_NEWS
                && !matches(msgclient->tonick, tonick)
                && !matches(msgclient->toname, UserName(sptr))
                && !matches(msgclient->tohost, sptr->user->host)
                && (!matches(toname, msgclient->tonick)
                    || matches(toname, tonick)
                       && !mycmp(msgclient->tonick, tonick))      
                && (!*Message(msgclient)
                    || !matches(Message(msgclient), message))
                || !mycmp(tonick, "admin.users"))
            && !matches(tohost, msgclient->fromhost)
            && !(msgclient->flags & FLAGS_KEY_TO_OPEN_OPER_LOCKS)) {
            c = wild_fromnick(msgclient->fromnick, msgclient);            
            sprintf(anyname, "%s!%s@%s", 
                    c ? c : msgclient->fromnick, msgclient->fromname, 
                    local_host(msgclient->fromhost));
            ret = msg_send(sptr, 1, passwd, flag_s, 
                           timeout_s, anyname, buf);
            if (!ret) joined++;
             else if (ret < 0) return;
               else { 
                     queued++;
                     t = fnl_msgclient(msgclient);
		 }
	 }
       t++;
   }
 strcpy(buf, "user");
 if (queued > 1) strcat(buf, "s"); 
 if (joined) { 
    strcat(buf, ", ");
    strcat(buf, myitoa(joined)); strcat(buf, " joined");
  } 
 sendto_one(sptr,"NOTICE %s :### News to %d %s", sptr->name, queued, buf);
}

static void msg_list(sptr, arg, passwd, flag_s, id_s, name, time_s)
aClient *sptr;
char *arg, *passwd, *flag_s, *id_s, *name, *time_s;
{
 aMsgClient *msgclient;
 int number = 0, t, last, ls = 0, count = 0, 
     found = 0, log = 0, id, id_count = 0;
 long clock, flags = 0, time_l, time_queued;
 char tonick[BUF_LEN], toname[BUF_LEN], tohost[BUF_LEN],
      *message, buf[BUF_LEN], dibuf[40], mbuf[MSG_LEN], 
      *dots = "...", *wildcard = "*";

 if (MyEq(arg,"ls")) ls = 1; else
 if (MyEq(arg,"count")) count = 1; else
 if (MyEq(arg,"log")) log = 1; else
 if (MyEq(arg,"llog")) log = 3;
  else {
         sendto_one(sptr,"NOTICE %s :#?# No such option: %s",sptr->name,arg); 
         return;
    }
 if (!*name && !id_s && !*flag_s) {
              if (log) log++;
              if (ls) ls++;
              name = wildcard;
  }
 if (!set_flags(sptr,flag_s, &flags,'d',"")) return;
 if (!*time_s) time_l = 0; 
  else { 
        time_l = set_date(sptr,time_s);
        if (time_l < 0) return;
    }
 split(name, tonick, toname, tohost);
 if (id_s) id = atoi(id_s); else id = 0;
 t = first_fnl_indexnode(UserName(sptr));
 last = last_fnl_indexnode(UserName(sptr));
 time(&clock); 
 while (last && t <= last) {
        msgclient = FromNameList[t];
	msgclient->id = ++id_count;
        flags = msgclient->flags;
        if (clock > msgclient->timeout) { t++; continue; }
        set_flags(sptr,flag_s,&flags,'d',"");
        if (local_check(sptr,msgclient,passwd,flags,
                           tonick,toname,tohost,time_l,id)) {
            message = Message(msgclient); number++;
            if (ls == 2 && *message) message = dots;
            display_flags(msgclient->flags, dibuf, '-');
            if (log) { 
                if (!(msgclient->flags & FLAGS_DISPLAY_IF_DEST_REGISTER))
                    { t++ ; continue; }
                strcpy(mbuf, get_msg(msgclient, 'n'));
                if (*mbuf == '-') *mbuf = '\0';
                if (*mbuf) { 
                   strcat(mbuf, "!");
                   strcat(mbuf, get_msg(msgclient, 'u')); strcat(mbuf, "@");
                   strcat(mbuf, get_msg(msgclient, 'h')); 
                   time_queued = atoi(get_msg(msgclient, '1'))+msgclient->time;
                 }
                if (log == 1 || log == 3) {            
                   found++;
                   if (*mbuf) {
                      strcat(mbuf, " (");
                      strcat(mbuf, get_msg(msgclient, 'r')); strcat(mbuf, ")");
                    } else {
                             time_queued = clock;
                             strcpy(mbuf, "<No matches yet>");
		        }
                   sendto_one(sptr,"NOTICE %s :### %s: %s!%s@%s => %s", 
                              sptr->name, log == 1 ? mytime(time_queued) :
                              myctime(time_queued),
                              msgclient->tonick, msgclient->toname, 
                              msgclient->tohost, mbuf); 
	         } else if (*mbuf) {
                            found++;
                            sendto_one(sptr,"NOTICE %s :### %s: %s", 
                                       sptr->name, 
                                       log == 2 ? mytime(time_queued) :
                                       myctime(time_queued), mbuf); 
		        }
	      } else 
                 if (!count) {
                    found++;
                    sprintf(buf,"for %s", 
                           relative_time(msgclient->timeout-clock));
                    sendto_one(sptr,"NOTICE %s :#%d %s %s (%s@%s) %s: %s",
                              sptr->name, msgclient->id, dibuf,
                              msgclient->tonick, msgclient->toname,
                              msgclient->tohost, buf, message);
		 }
	 }
       t++;
     }
 if (count) sendto_one(sptr,"NOTICE %s :### %s %s (%s@%s): %d",
                       sptr->name,"Number of requests to",
                       tonick ? tonick : "*", toname ? toname:"*",
                       tohost ? tohost : "*", number);
  else if (!found) sendto_one(sptr,"NOTICE %s :#?# No such %s", sptr->name,
                              log ? "log(s)" : "request(s) found");
}

static void msg_flag(sptr, passwd, flag_s, id_s, name, newflag_s)
aClient *sptr;
char *passwd, *flag_s, *id_s, *name, *newflag_s;
{
 aMsgClient *msgclient;
 int flagged = 0, t, last, id;
 long clock, flags = 0;
 char tonick[BUF_LEN], toname[BUF_LEN], tohost[BUF_LEN],
      dibuf1[40], dibuf2[40];

 if (!*newflag_s) {
     sendto_one(sptr,"NOTICE %s :#?# No flag changes specified",sptr->name);
     return;
  }
 if (!set_flags(sptr, flag_s, &flags,'d',"in matches flag")) return;
 if (!set_flags(sptr, newflag_s, &flags,'c',"in flag changes")) return;
 split(name, tonick, toname, tohost);
 if (id_s) id = atoi(id_s); else id = 0;
 t = first_fnl_indexnode(UserName(sptr));
 last = last_fnl_indexnode(UserName(sptr));
 time(&clock);
 while (last && t <= last) {
       msgclient = FromNameList[t];flags = msgclient->flags;
        if (clock > msgclient->timeout) { t++; continue; }
        set_flags(sptr,flag_s,&flags,'d',"");
        if (local_check(sptr,msgclient,passwd,flags,
                        tonick,toname,tohost,0,id)) {
            flags = msgclient->flags; display_flags(flags, dibuf1, '-');
            set_flags(sptr,newflag_s,&msgclient->flags,'s',"");
            display_flags(msgclient->flags, dibuf2, '-');
            if (flags == msgclient->flags) 
                sendto_one(sptr,"NOTICE %s :### %s -> %s %s (%s@%s)",
                           sptr->name, "No flag change for",
                           dibuf1, msgclient->tonick,
                           msgclient->toname,msgclient->tohost);
             else                                        
                sendto_one(sptr,"NOTICE %s :### %s -> %s %s (%s@%s) to %s",
                          sptr->name,"Flag change",dibuf1,msgclient->tonick,
                          msgclient->toname, msgclient->tohost, dibuf2);
           flagged++;
        } 
       t++;
   }
 if (!flagged) 
  sendto_one(sptr,"NOTICE %s :#?# No such request(s) found",sptr->name);
}

static void msg_sent(sptr, arg, name, time_s, delete)
aClient *sptr;
char *arg, *name, *time_s, *delete;
{
 aMsgClient *msgclient,*next_msgclient;
 char fromnick[BUF_LEN], fromname[BUF_LEN], fromhost[BUF_LEN]; 
 int number = 0, t, t1, last, nick = 0, count = 0, users = 0;
 long clock,time_l;

 if (!*arg) nick = 1; else
  if (MyEq(arg,"COUNT")) count = 1; else
   if (MyEq(arg,"USERS")) users = 1; else
   if (!MyEq(arg,"NAME")) {
       sendto_one(sptr,"NOTICE %s :#?# No such option: %s",sptr->name, arg); 
       return;
  }
 if (users) {
    if (!IsOperHere(sptr)) {
        sendto_one(sptr,"NOTICE %s :### %s",sptr->name,
                   "A dragon is guarding the names...");
        return;
     }
    if (!*time_s) time_l = 0; 
     else { 
            time_l = set_date(sptr,time_s);
            if (time_l < 0) return;
       }
    split(name, fromnick, fromname, fromhost); 
    time(&clock);
    for (t = 1; t <= fromname_index; t++) {
         msgclient = FromNameList[t]; t1 = t;
         do next_msgclient = t1 < fromname_index ? FromNameList[++t1] : 0;
          while (next_msgclient && next_msgclient->timeout <= clock);
         if (msgclient->timeout > clock
             && (!time_l || msgclient->time >= time_l 
                 && msgclient->time < time_l+24*3600)
             && (!matches(fromnick,msgclient->fromnick))
             && (!matches(fromname,msgclient->fromname))
             && (!matches(fromhost,msgclient->fromhost))
             && (!*delete || !mycmp(delete,"RM")
                 || !mycmp(delete,"RMBF") &&
                    (msgclient->flags & FLAGS_RETURN_CORRECT_DESTINATION ||
                     msgclient->flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE))) {
             if (*delete || !next_msgclient
                 || mycmp(next_msgclient->fromnick,msgclient->fromnick)
                 || mycmp(next_msgclient->fromname,msgclient->fromname)     
                 || !(host_check(next_msgclient->fromhost,
                                 msgclient->fromhost))) {
                  sendto_one(sptr,"NOTICE %s :### %s[%d] %s (%s@%s) @%s",
                             sptr->name,
                             *delete ? "Removing -> " : "", 
                             *delete ? 1 : count+1,
                             msgclient->fromnick,msgclient->fromname,
                             local_host(msgclient->fromhost),
                             msgclient->fromhost);
                    if (*delete) msgclient->timeout = clock-1;
                 count = 0; number = 1;
	     } else count++;
	 }
     }
    if (!number) 
     sendto_one(sptr, "NOTICE %s :#?# No request(s) from such user(s) found",
                sptr->name);
  return;
 }
 t = first_fnl_indexnode(UserName(sptr));
 last = last_fnl_indexnode(UserName(sptr));
 time (&clock);
 while (last && t <= last) {
        msgclient = FromNameList[t];
        if (clock > msgclient->timeout) { t++; continue; }
        if (!Usermycmp(UserName(sptr),msgclient->fromname)
            && (!nick || !mycmp(sptr->name,msgclient->fromnick))) {
            if (host_check(sptr->user->host,msgclient->fromhost)) { 
                if (!count) 
                    sendto_one(sptr,"NOTICE %s :### Queued %s from host %s",
                               sptr->name, mytime(msgclient->time),
                               msgclient->fromhost);
                number++;
             }
         }
       t++;
    }
 if (!number) 
  sendto_one(sptr,"NOTICE %s :#?# No such request(s) found",sptr->name);
  else if (count) sendto_one(sptr,"NOTICE %s :### %s %d",sptr->name,
                             "Number of requests queued:",number);
}

static int name_len_error(sptr, name)
aClient *sptr;
char *name;
{
 if (strlen(name) >= BUF_LEN) {
    sendto_one(sptr,
               "NOTICE %s :#?# Nick!name@host can't be longer than %d chars",
               sptr->name, BUF_LEN-1);
    return 1;
  }
 return 0;
}

static int flag_len_error(sptr, flag_s)
aClient *sptr;
char *flag_s;
{
 if (strlen(flag_s) >= BUF_LEN) {
    sendto_one(sptr,"NOTICE %s :#?# Flag string can't be longer than %d chars",
               sptr->name,BUF_LEN-1);
    return 1;
  }
 return 0;
}

static int alias_send(sptr, option, flags, msg, timeout)
aClient *sptr;
char *option, **flags, **msg, **timeout;
{
 static char flag_s[BUF_LEN+10], *deft = "+31",
        *waitfor_message = "[Waiting]";
 
 if (MyEq(option,"SEND")) {
     sprintf(flag_s,"+D%s", *flags);
     if (!*timeout) *timeout = deft; 
  } else
 if (MyEq(option,"NEWS")) {
     sprintf(flag_s,"+RS%s", *flags);
     if (!*timeout) *timeout = deft; 
  } else
 if (MyEq(option,"WAITFOR")) { 
     sprintf(flag_s,"+YD%s", *flags); 
     if (!*msg) *msg = waitfor_message; 
  } else 
 if (MyEq(option,"SPY")) sprintf(flag_s,"+RX%s", *flags); else
 if (MyEq(option,"FIND")) sprintf(flag_s,"+FR%s", *flags); else
 if (MyEq(option,"KEY")) sprintf(flag_s,"+KR%s", *flags); else
 if (MyEq(option,"WALL")) sprintf(flag_s,"+BR%s", *flags); else
 if (MyEq(option,"WALLOPS")) sprintf(flag_s,"+BRW%s", *flags); else
 if (MyEq(option,"DENY")) sprintf(flag_s,"+RZ%s", *flags); else
  return 0;
 *flags = flag_s;
 return 1;
}

static void antiwall(sptr)
aClient *sptr;
{
 int t = 1, wall = 0;
 aMsgClient *msgclient;
 long gflags = 0, clock;
 char buf[MSG_LEN], *c;

 time (&clock);
 if (!IsOper(sptr)) { 
     sendto_one(sptr,"NOTICE %s :#?# Only ghosts may travel through walls...", 
               sptr->name);
     return;
  }
 while (fromname_index && t <= fromname_index) {
        msgclient = FromNameList[t];
        if (msgclient->flags & FLAGS_FIND_CORRECT_DEST_SEND_ONCE 
            && !matches(msgclient->tonick, sptr->name)
            && !matches(msgclient->toname, UserName(sptr))
            && !matches(msgclient->tohost, sptr->user->host)
            && flag_send((aClient *)0, sptr, (aClient *)0, sptr->name, 
                         msgclient, '-', NULLCHAR)) {
            msgclient->flags &= ~FLAGS_FIND_CORRECT_DEST_SEND_ONCE;
            sendto_one(sptr,
                       "NOTICE %s :### Note wall to %s!%s@%s deactivated.", 
                       sptr->name, msgclient->tonick, msgclient->toname, 
                       msgclient->tohost);
            sprintf(buf,"%s (%s@%s) has deactivated your Note Wall...",
                    sptr->name,UserName(sptr),sptr->user->host);
            c = wild_fromnick(msgclient->fromnick, msgclient);
            gflags |= FLAGS_WASOPER;
            gflags |= FLAGS_SERVER_GENERATED_NOTICE;
            new(msgclient->passwd,"SERVER","-","-",
                c ? c : msgclient->fromnick, msgclient->fromname,
                local_host(msgclient->fromhost), gflags, 
                           note_mst*24*3600+clock, clock, buf);
            wall = 1;
	 }
        t++;
    }
 if (!wall) { 
     sendto_one(sptr,"NOTICE %s :#?# Hunting for ghost walls?", 
                sptr->name);
     return;
  }
}

int m_note(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
 char *option, *arg, *timeout = NULLCHAR, *passwd = NULLCHAR, *param,  
      *id = NULLCHAR, *flags = NULLCHAR, *name = NULLCHAR, *msg = NULLCHAR,
      *wildcard = "*", *c, *c2, *default_timeout = "+1", *deft = "+31";
 static char buf1[BUF_LEN], buf2[BUF_LEN], buf3[BUF_LEN],
        buf4[BUF_LEN], msg_buf[MSG_LEN], passwd_buf[BUF_LEN], 
        timeout_buf[BUF_LEN], option_buf[BUF_LEN], 
        id_buf[BUF_LEN], flags_buf[BUF_LEN + 3];
 int t, silent = 0, remote = 0;
 aClient *acptr;

 if (!file_inited) init_messages();
 if (parc < 2) {
    sendto_one(sptr,"NOTICE %s :#?# No option specified.", sptr->name); 
    return -1;
  }
 param = parv[1];
 if (parc > 1 && !myncmp(param,"service ",8)) {
     if (!IsOperHere(sptr)) {
         sendto_one(sptr,"NOTICE %s :### %s",sptr->name,
                   "Beyond your power poor soul...");
         return 0;
      }
     sptr = find_person(split_string(param, 2, 1), (aClient *)0);
     if (!sptr) return 0;
     t = 0; 
     while (*param && t < 2) {
            if (*param == ' ') t++;
            param++;
       }                 
     if (!*param) parc = 0;
  }  else {
           c = split_string(param, 1, 1); c2 = c;
           while (*c2 && *c2 != '.') c2++;
           if (*c2) {
               if (wildcards(c))
                   for (t = 0; t <= highest_fd; t++) {
                       if (!(acptr = local[t])) continue;
                       if (IsServer(acptr) && acptr != cptr) 
                          sendto_one(acptr, ":%s NOTE %s", sptr->name, param);
		    }
                 else for (acptr = client; acptr; acptr = acptr->next)
                          if (IsServer(acptr) && acptr != cptr
                              && !mycmp(c, acptr->name)) {
		              sendto_one(acptr, 
                                        ":%s NOTE %s", sptr->name, param);
                              break;
			   }
               if (!matches(c, me.name)) {
                   if (wildcards(c)) remote = -1; else remote = 1;
                   while (*param && *param != ' ') param++; 
                         if (*param) param++;
                         if (!*param) {
                            sendto_one(sptr,
                                       "NOTICE %s :#?# No option specified.", 
                                       sptr->name);
                            return -1;
                          }
	       } else return 0;
     
	    }
       }
 if (!IsRegistered(sptr)) { 
	sendto_one(sptr, ":%s %d * :You have not registered as an user", 
		   me.name, ERR_NOTREGISTERED); 
	return -1;
   }
 if (strlen(param) >= MSG_LEN) {
    sendto_one(sptr,"NOTICE %s :#?# Line can't be longer than %d chars",
               sptr->name, MSG_LEN-1);
    return -1;
 }
 for (t = 2; t < 10; t++) {
      arg = split_string(param, t, 1);
      switch (*arg) {
              case '&' :
              case '$' : passwd = passwd_buf;strncpyzt(passwd,arg+1,10);
                         break; 
              case '%' : passwd = passwd_buf;strncpyzt(passwd,arg+1,10);
                         silent = 1; break;
              case '#' : id = id_buf; strncpyzt(id,arg+1,BUF_LEN); break; 
              case '+' :
              case '-' : if (numeric(arg+1)) {
                             timeout = timeout_buf; 
                             strncpyzt(timeout,arg,BUF_LEN); 
			  } else { 
                                   flags = flags_buf;
                                   strncpyzt(flags,arg,BUF_LEN);  
			     }
                          break;
              default : goto end_loop_case;
        }
    } 
 end_loop_case:;
 strncpyzt(option_buf, split_string(param, 1, 1), BUF_LEN-1); 
 strncpyzt(buf1, split_string(param, t, 1), BUF_LEN-1);
 strncpyzt(buf2, split_string(param, t+1, 1), BUF_LEN-1);
 strncpyzt(buf3, split_string(param, t+2, 1), BUF_LEN-1);
 strncpyzt(buf4, split_string(param, t+3, 1), BUF_LEN-1);
 strcpy(msg_buf, split_string(param, t+1, 0)); msg = msg_buf;
 option = option_buf;
 c = msg; while (*c) c++;
 while (c != msg && *--c == ' '); 
 if (c != msg) c++; *c = 0;
 if (!*msg) msg = NULLCHAR;
 if (!passwd || !*passwd) passwd = wildcard;
 if (!flags) flags = (char *) "";
 if (flags && flag_len_error(sptr, flags)) return 0;
 time (&old_clock_request);

 if (MyEq(option,"STATS")) msg_stats(sptr, buf1, buf2); else
 if (MyEq(option,"VERSION")) sendto_one(sptr,"NOTICE %s :Running version %s", 
                                        sptr->name, VERSION); 
 else if (remote < 0 && (mycmp(option, "NEWS") || !msg)) return 0; else
 if (alias_send(sptr,option, &flags, &msg, &timeout) || MyEq(option,"USER")) {
     if (!*buf1) {
        if (MyEq(option,"SPY")) check_messages(sptr, sptr, sptr->name, 'g');
         else sendto_one(sptr,
                       "NOTICE %s :#?# Please specify at least one argument", 
                        sptr->name);
        return 0;
      }
     name = buf1;if (!*name) name = wildcard;
     if (name_len_error(sptr, name)) return 0;
     if (!timeout || !*timeout) timeout = default_timeout; 
     if (mycmp(option, "NEWS") || !msg) {
         msg_send(sptr, silent, passwd, flags, timeout, name, msg);
      } else msg_news(sptr, silent, passwd, flags, 
                      timeout == default_timeout ? deft : timeout, 
                      name, msg);
  } else
 if (MyEq(option,"LS") || MyEq(option,"COUNT") 
     || MyEq(option,"LOG") || MyEq(option,"LLOG")) {
     name = buf1;if (name_len_error(sptr, name)) return 0;
     msg_list(sptr, option, passwd, flags, id, name, buf2);
   } else
 if (MyEq(option,"SAVE")) msg_save(sptr); else
 if (MyEq(option,"ANTIWALL")) antiwall(sptr); else
 if (MyEq(option,"FLAG")) {
     name = buf1;if (!*name) name = wildcard;
     if (name_len_error(sptr, name)) return 0;
     msg_flag(sptr, passwd, flags, id, name, buf2); 
  } else
 if (MyEq(option,"SENT")) {
    name = buf2;if (!*name) name = wildcard;
    if (name_len_error(sptr, name)) return 0;
    msg_sent(sptr, buf1, name, buf3, buf4);
  } else
 if (!mycmp(option,"RM")) {
     if (!*buf1 && !id && !*flags) {
        sendto_one(sptr,
                   "NOTICE %s :#?# Please specify at least one argument", 
                   sptr->name);
        return 0;
      }
     name = buf1;if (!*name) name = wildcard;
     if (name_len_error(sptr, name)) return 0;
     msg_remove(sptr, passwd, flags, id, name, buf2); 
  } else sendto_one(sptr,"NOTICE %s :#?# No such option: %s", 
                    sptr->name, option);
 return 0;
}
#endif
