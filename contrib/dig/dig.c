/*	BSDI $Id: dig.c,v 1.2 1993/12/17 03:39:11 sanders Exp $	*/

 /*******************************************************************
 **      DiG -- Domain Information Groper                          **
 **                                                                **
 **        dig.c - Version 2.0 (9/1/90)                            **
 **                                                                **
 **        Developed by: Steve Hotz & Paul Mockapetris             **
 **        USC Information Sciences Institute (USC-ISI)            **
 **        Marina del Rey, California                              **
 **        1989                                                    **
 **                                                                **
 **        dig.c -                                                 **
 **           Version 2.0 (9/1/90)                                 **
 **               o renamed difftime() difftv() to avoid           **
 **                 clash with ANSI C                              **
 **               o fixed incorrect # args to strcmp,gettimeofday  **
 **               o incorrect length specified to strncmp          **
 **               o fixed broken -sticky -envsa -envset functions  **
 **               o print options/flags redefined & modified       **
 **                                                                **
 **           Version 2.0.beta (5/9/90)                            **
 **               o output format - helpful to `doc`               **
 **               o minor cleanup                                  **
 **               o release to beta testers                        **
 **                                                                **
 **           Version 1.1.beta (10/26/89)                          **
 **               o hanging zone transer (when REFUSED) fixed      **
 **               o trailing dot added to domain names in RDATA    **
 **               o ISI internal                                   **
 **                                                                **
 **           Version 1.0.tmp  (8/27/89)                           **
 **               o Error in prnttime() fixed                      **
 **               o no longer dumps core on large pkts             **
 **               o zone transfer (axfr) added                     **
 **               o -x added for inverse queries                   **
 **                               (i.e. "dig -x 128.9.0.32")       **
 **               o give address of default server                 **
 **               o accept broadcast to server @255.255.255.255    **
 **                                                                **
 **           Version 1.0  (3/27/89)                               **
 **               o original release                               **
 **                                                                **
 **     DiG is Public Domain, and may be used for any purpose as   **
 **     long as this notice is not removed.                        **
 ****                                                            ****
 ****   NOTE: Version 2.0.beta is not for public distribution    ****
 ****                                                            ****
 *******************************************************************/


#define VERSION 20
#define VSTRING "2.0"

#include "hfiles.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include RESOLVH
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h> 
#include <errno.h>
#include "subr.c"
#include "pflag.h"
#include "options.c"

#ifdef RESLOCAL
#include "qtime.c"
#else
#include "qtime.h"
#endif

int pfcode = PRF_DEF;
int eecode = 0;
extern char *inet_ntoa();
extern struct state _res;

FILE  *qfp;
int sockFD;

#define MAXDATA		256   
#define SAVEENV "DiG.env"

char *defsrv, *srvmsg;
char defbuf[40] = "default -- ";
char srvbuf[60];

#define UC(b)   (((int)b)&0xff)

 /*
 ** Take arguments appearing in simple string (from file)
 ** place in char**.
 */
stackarg(y,l)
     char *l;
     char **y;
{
  int done=0;
  while (!done) {
    switch (*l) {
    case '\t':
    case ' ':  l++;    break;
    case NULL:
    case '\n': done++; *y = NULL; break;
    default:   *y++=l;  while (!isspace(*l)) l++;
      if (*l == '\n') done++; *l++ = NULL; *y = NULL;
}}}


main(argc, argv)
     int argc;
     char **argv;
{
	struct hostent *hp;
	short port = htons(NAMESERVER_PORT);
	char packet[PACKETSZ];
	char answer[PACKETSZ];
	int n;
	char doping[90];
	char pingstr[50];
	char *afile;
	unsigned long tmpaddr;
        char revaddr[10][10];
        int addri, addrj;
	char *addrc;

	struct timeval exectime, tv1,tv2,tv3;
	char curhost[30];

	char *srv;
	int anyflag = 0;
	int sticky = 0;
	int tmp; 
	int qtype = 1, qclass = 1;
	int addrflag = 0;
	int zone = 0;
        int bytes_out, bytes_in;

	char cmd[256];
	char domain[128];
        char msg[120], *msgptr;
	char **vtmp;
	char *args[30];
	char **ax;
	char **ay;
	int once = 1, dofile=0; /* batch -vs- interactive control */
	char fileq[100];
	char *qptr;
	int  fp;
	int wait=0;
	int envset=0, envsave=0;
	int Xpfcode, Tpfcode, Toptions;
	char *Xres, *Tres;
	char *pp;


	res_init();
	gethostname(curhost,30);
	defsrv = strcat(defbuf, inet_ntoa(_res.nsaddr.sin_addr));
	_res.options |= RES_DEBUG;
	Xres = (char *) malloc(sizeof(_res)+1);
	Tres = (char *) malloc(sizeof(_res)+1);
	bcopy(&_res,Xres,sizeof(struct state));
	Xpfcode=pfcode;

 /*
 ** If LOCALDEF in environment, should point to file
 ** containing local favourite defaults.  Also look for file
 ** DiG.env (i.e. SAVEENV) in local directory.
 */

	if ((((afile= (char *) getenv("LOCALDEF")) != (char *) NULL) &&
	     ((fp=open(afile,O_RDONLY)) > 0)) ||
	    ((fp = open(SAVEENV,O_RDONLY)) > 0)) {
	  read(fp,Xres,sizeof(struct state));
	  read(fp,&Xpfcode,sizeof(int));
	  close(fp);
	  bcopy(Xres,&_res,sizeof(struct state));
	  pfcode = Xpfcode;
	}
 /*
 **   check for batch-mode DiG 
 */
	vtmp = argv; ax=args;
	while (*vtmp != NULL) {
	  if (strcmp(*vtmp,"-f") == 0) {
	    dofile++; once=0;
	    if ((qfp = fopen(*++vtmp,"r")) == NULL) {
	      fflush(stdout);
	      perror("file open");
	      fflush(stderr);
	      exit(10);
	    }
	  } else {
	    *ax++ = *vtmp;
	  }
	  vtmp++;
	}

	_res.id = 1;
	gettimeofday(&tv1,NULL);

 /*
 **  Main section: once if cmd-line query
 **                while !EOF if batch mode
 */
	*fileq=NULL;
	while ((dofile && (fgets(fileq,100,qfp) !=NULL)) || 
	       ((!dofile) && (once--))) 
	  {
	    if ((*fileq=='\n') || (*fileq=='#') || (*fileq==';'))
	      continue; /* ignore blank lines & comments */

/*
 * "sticky" requests that before current parsing args
 * return to current "working" environment (X******)
 */
	    if (sticky) {
	      bcopy(Xres,&_res,sizeof(struct state));
	      pfcode = Xpfcode;
	    }

/* concat cmd-line and file args */
	    ay=ax;
	    qptr=fileq;
	    stackarg(ay,qptr);

	    /* defaults */
	    qtype=qclass=1;
	    zone = 0;
	    *pingstr=0;
	    srv=NULL;

	    sprintf(cmd,"\n; <<>> DiG %s <<>> ",VSTRING);
	    argv = args;
/*
 * More cmd-line options than anyone should ever have to
 * deal with ....
 */
	    while (*(++argv) != NULL) { 
	      strcat(cmd,*argv); strcat(cmd," ");
	      if (**argv == '@') {
		srv = (*argv+1);
		continue;
	      }
	      if (**argv == '%')
		continue;
	      if (**argv == '+') {
		SetOption(*argv+1);
		continue;
	      }
	 
	      if (strncmp(*argv,"-nost",5) == 0) {
		sticky=0;; continue;
	      } else if (strncmp(*argv,"-st",3) == 0) {
		sticky++; continue;
	      } else if (strncmp(*argv,"-envsa",6) == 0) {
		envsave++; continue;
	      } else if (strncmp(*argv,"-envse",6) == 0) {
		envset++; continue;
	      }

         if (**argv == '-') {
	   switch (argv[0][1]) { 
	   case 'T': wait = atoi(*++argv);
	     break;
	   case 'c': 
	     if ((tmp = atoi(*++argv)) || *argv[0]=='0') {
	       qclass = tmp;
	     } else if (tmp = StringToClass(*argv,0)) {
	       qclass = tmp;
	     } else {
	       printf("; invalid class specified\n");
	     }
	     break;
	   case 't': 
	     if ((tmp = atoi(*++argv)) || *argv[0]=='0') {
	       qtype = tmp;
	     } else if (tmp = StringToClass(*argv,0)) {
	       qtype = tmp;
	     } else {
	       printf("; invalid type specified\n");
	     }
	     break;
	   case 'x':
	     if (qtype == T_A) qtype = T_ANY;
	     if ((addrc = *++argv) == (char *)0) {
	       printf(";; -x requires an argument\n");
	       continue;
	     }
	     addri=addrj=0;
	     while (*addrc) {
	       if (*addrc == '.') {
		 revaddr[addri][addrj++] = '.';
		 revaddr[addri][addrj] = (char) 0;
		 addri++; addrj=0;
	       } else {
		 revaddr[addri][addrj++] = *addrc;
	       }
	       addrc++;
	     }
	     if (*(addrc-1) == '.') {
	       addri--;
	     } else {
	       revaddr[addri][addrj++] = '.';
	       revaddr[addri][addrj] = (char) 0;
	     }
	     *domain = (char) 0;
	     for (addrj=addri; addrj>=0; addrj--)
	       strcat(domain,revaddr[addrj]);
	     strcat(domain,"in-addr.arpa.");
/* old code -- some bugs
	     tmpaddr = inet_addr(*++argv);
	     pp = (char *) &tmpaddr;
	     sprintf(domain,"%d.%d.%d.%d.in-addr.arpa",
		     UC(pp[3]), UC(pp[2]), UC(pp[1]), UC(pp[0]));
*/        
	     break;
	   case 'p': port = htons(atoi(*++argv)); break;
	   case 'P':
	     if (argv[0][2] != NULL)
	       strcpy(pingstr,&argv[0][2]);
	     else
	       strcpy(pingstr,"ping -s");
	     break;
	   } /* switch - */
	   continue;
	 } /* if '-'   */

	      if ((tmp = StringToType(*argv,-1)) != -1) { 
		if ((T_ANY == tmp) && anyflag++) {  
		  qclass = C_ANY; 	
		  continue; 
		}
		if (T_AXFR == tmp) {
		  pfcode = PRF_ZONE;
		  zone++;
		} else {
		  qtype = tmp; 
		}
	      }
	      else if ((tmp = StringToClass(*argv,-1)) != -1) { 
		qclass = tmp; 
	      }	 else {
		bzero(domain,128);
		sprintf(domain,"%s",*argv);
	      }
	    } /* while argv remains */
if (pfcode & 0x80000)
  printf("; pflag: %x res: %x\n", pfcode, _res.options);
	  
/*
 * Current env. (after this parse) is to become the
 * new "working environmnet. Used in conj. with sticky.
 */
	    if (envset) {
	      bcopy(&_res,Xres,sizeof(struct state));
	      Xpfcode=pfcode;
	      envset=0;
	    }

/*
 * Current env. (after this parse) is to become the
 * new default saved environmnet. Save in user specified
 * file if exists else is SAVEENV (== "DiG.env").
 */
	    if (envsave) {
	      if ((((afile= (char *) getenv("LOCALDEF")) != (char *) NULL) &&
		   ((fp=open(afile,O_WRONLY|O_CREAT|O_TRUNC,
			     S_IREAD|S_IWRITE)) > 0)) ||
		  ((fp = open(SAVEENV,O_WRONLY|O_CREAT|O_TRUNC,
			       S_IREAD|S_IWRITE)) > 0)) {
		write(fp,&_res,sizeof(struct state));
		write(fp,&pfcode,sizeof(int));
		close(fp);
	      }
	      envsave=0;
	    }

	    if (pfcode & PRF_CMD)
	      printf("%s\n",cmd);

      addrflag=anyflag=0;

/*
 * Find address of server to query. If not dot-notation, then
 * try to resolve domain-name (if so, save and turn off print 
 * options, this domain-query is not the one we want. Restore
 * user options when done.
 * Things get a bit wierd since we need to use resolver to be
 * able to "put the resolver to work".
 */

   srvbuf[0]=0;
   srvmsg=defsrv;
   if (srv != NULL) {
     tmpaddr = inet_addr(srv);
     if ((tmpaddr != (unsigned)-1) || 
	 (strncmp("255.255.255.255",srv,15) == 0)) {
       _res.nscount = 1;
       _res.nsaddr.sin_addr.s_addr = tmpaddr;
       srvmsg = strcat(srvbuf, srv);
     } else {
      Tpfcode=pfcode;
      pfcode=0;
      bcopy(&_res,Tres,sizeof(_res));
      Toptions = _res.options;
      _res.options = RES_DEFAULT;
      res_init();
      hp = gethostbyname(srv);
      pfcode=Tpfcode;
      if (hp == NULL) {
	fflush(stdout);
	fprintf(stderr,
		"; Bad server: %s -- using default server and timer opts\n",
		srv);
	fflush(stderr);
        srvmsg = defsrv;
	srv = NULL;
	_res.options = Toptions;
      } else {
	bcopy(Tres,&_res,sizeof(_res));
	bcopy(hp->h_addr, &_res.nsaddr_list[0].sin_addr, hp->h_length);
	_res.nscount = 1;
        srvmsg = strcat(srvbuf,srv);
        strcat(srvbuf, "  ");
        strcat(srvmsg,inet_ntoa(_res.nsaddr.sin_addr));
      }
    }
     _res.id += _res.retry;
   }

       _res.id += _res.retry;
/*       _res.options |= RES_DEBUG;  */
       _res.nsaddr.sin_family = AF_INET;
       _res.nsaddr.sin_port = port;

       if (zone) {
	 do_zone(domain,qtype);
	 if (pfcode & PRF_STATS) {
	   gettimeofday(&exectime,NULL);
	   printf(";; FROM: %s to SERVER: %s\n",curhost,srvmsg);
	   printf(";; WHEN: %s",ctime(&(exectime.tv_sec)));
	 }
	  
	 fflush(stdout);
	 continue;
       }

       bytes_out = n = res_mkquery(QUERY, domain, qclass, qtype,
                         (char *)0, 0, NULL, packet, sizeof(packet));
       if (n < 0) {
	 fflush(stderr);
	 printf(";; res_mkquery: buffer too small\n\n");
	 continue;
       }
       eecode = 0;
       if ((bytes_in = n = res_send(packet, n, answer, sizeof(answer))) < 0) {
	 fflush(stdout);
         n = 0 - n;
         msg[0]=0;
         strcat(msg,";; res_send to server ");
         strcat(msg,srvmsg);
	 perror(msg);
	 fflush(stderr);

#ifndef RESLOCAL
/*
 * resolver does not currently calculate elapsed time
 * if there is an error in res_send()
 */
/*
	 if (pfcode & PRF_STATS) {
	   printf(";; Error time: "); prnttime(&hqtime); putchar('\n');
	 }
*/
#endif
	 if (!dofile) {
            if (eecode)
	      exit(eecode);
	    else
	      exit(9);
	  }
       }
#ifndef RESLOCAL
       else {
	 if (pfcode & PRF_STATS) {
	   printf(";; Sent %d pkts, answer found in time: ",hqcount);
	   prnttime(&hqtime);
	   if (hxcount)
	     printf(" sent %d too many pkts",hxcount);
	   putchar('\n');
	 }
       }
#endif

       if (pfcode & PRF_STATS) {
	 gettimeofday(&exectime,NULL);
	 gethostname(curhost,30);
	 printf(";; FROM: %s to SERVER: %s\n",curhost,srvmsg);
	 printf(";; WHEN: %s",ctime(&(exectime.tv_sec)));
         printf(";; MSG SIZE  sent: %d  rcvd: %d\n", bytes_out, bytes_in);
       }
	  
       fflush(stdout);
/*
 *   Argh ... not particularly elegant. Should put in *real* ping code.
 *   Would necessitate root priviledges for icmp port though!
 */
       if (*pingstr) {
         sprintf(doping,"%s %s 56 3 | tail -3",pingstr,
		 (srv==NULL)?(defsrv+10):srv);
	 system(doping);
       }
       putchar('\n');

/*
 * Fairly crude method and low overhead method of keeping two
 * batches started at different sites somewhat synchronized.
 */
       gettimeofday(&tv2, NULL);
       tv1.tv_sec += wait;
       difftv(&tv1,&tv2,&tv3);
       if (tv3.tv_sec > 0)
	 sleep(tv3.tv_sec);
	  }
	return(eecode);
      }
