/* Copyright (C) 1990, 1993  Free Software Foundation, Inc.

   This file is part of GNU ISPELL.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* ISPELL is an interactive spelling corrector generates a list
   of near misses for each misspelled word.  The direct parantage
   of this version seems to have begun in California in the
   early 70's.  A child of that version was known as SPELL on
   the MIT ITS systems, and later ISPELL on MIT TOPS-20 systems.
   I wrote the first C implementation in 1983, and then developed
   this version in 1988.  See the History section of ispell.texinfo
   for more information.
 
   Pace Willisson
   pace@blitz.com

   See ChangeLog for further author information. */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "getopt.h"

#include "ispell.h"
#include "hash.h"
#include "charset.h"

static void show_warranty (NOARGS);
static void show_copying (NOARGS);
static void show_signon (NOARGS);

char **lexdecode = NULL;

/* this array contains letters to use when generating near misses */
char near_miss_letters[256];
int nnear_miss_letters;

/* this array has 1 for any character that is in near_miss_letters */
char near_map[256];

enum formatter formatter;

/* set to 1 if any interaction takes place */
int interaction_flag = 1;

int lflag;
int aflag;
int dflag;
int pflag;
int Sflag;
int askflag;
int Dflag;
int Eflag;
int hflag;
int uflag;
int iflag;
char *dname;
char *privname;
int intr_typed;

FILE *sortf;


void
usage ()
{
  (void) fprintf (stderr, "Usage: ispell [-d dict] [-p privdict] file\n");
  (void) fprintf (stderr, "or:    ispell [-a | -l]\n");
  exit (1);
}


RETSIGTYPE
intr ()
{
  signal (SIGINT, (RETSIGTYPE (*)()) intr);
  intr_typed = 1;
}


void
done ()
{
  extern char tempfile[];

  if (tempfile[0])
    {
      (void) unlink (tempfile);
      tempfile[0] = 0;
    }
  termuninit ();
  exit (0);
}


int
main (argc, argv)
  int argc;
  char **argv;
{
  int c;
  extern char *optarg;
  extern int optind;
  char *p;
  int i;

  for (i = 0; i < 256; i++)
    if (charset[i].word_component)
      (ctbl + 1)[i] |= LEXLETTER;

  /* if invoked by the name 'spell', set uflag */
  if ((p = (char *) strrchr (argv[0], '/')))
    p++;
  else
    p = argv[0];

  if (strcmp (p, "spell") == 0)
    {
      uflag = 1;
      lflag = 1;
    }

  formatter = formatter_generic;

  while ((c = getopt (argc, argv, "alp:d:SDEhuvbixtWC")) != EOF)
    {
      switch (c)
	{
	case 'W':
	  show_warranty ();
	  exit (0);
	case 'C':
	  show_copying ();
	  exit (0);
	case 't':
	  formatter = formatter_tex;
	  break;
	  /* traditional spell options */
	  /* ignore these */
	case 'v':		/* don't trust original suffix stripper */
	case 'b':		/* check british spelling */
	  break;
	case 'i':		/* don't follow any .so's or .nx's */
	  iflag = 1;
	  break;
	  /* case 'l': */
	  /* follow nroff .so's, and .nx's into /usr/lib */
	  /* in ispell, this means make a list.  luckily,
			 * if the user wants old style, lflag is turned
			 * on anyway
			 */

	case 'x':		/* print stems with '='s */
	  (void) fprintf (stderr, "-x is not supported\n");
	  exit (1);

	  /* end of traditional options */

	case 'u':
	  uflag = 1;
	  lflag = 1;
	  break;
	case 'h':
	  hflag = 1;
	  break;
	default:
	  usage ();
	case 'D':
	  Dflag = 1;
	  break;
	case 'E':
	  Eflag = 1;
	  break;
	case 'p':
	  pflag = 1;
	  privname = optarg;
	  break;
	case 'd':
	  dflag = 1;
	  dname = optarg;
	  break;
	case 'a':
	  aflag = 1;
	  break;
	case 'l':
	  lflag = 1;
	  break;
	case 'S':
	  Sflag = 1;
	  break;
	}
    }

  if (optind < argc && argv[optind][0] == '+')
    {
      /* this is for traditional spell - add words in
		 * named file to OK list
		 */
      (void) p_load (argv[optind] + 1, 0);
      optind++;
    }

  if (dflag == 0)
    {
      dname = (char *) getenv ("ISPELL_DICTIONARY");
      if (dname == NULL)
	{
	  dname = (char *) xmalloc ((unsigned) strlen (DICT_LIB) + 30);
	  (void) strcpy (dname, DICT_LIB);
	  (void) strcat (dname, "/ispell.dict");
	}
    }

  if (access (dname, 4) < 0)
    {
      (void) fprintf (stderr, "can't read dictionary %s\n", dname);
      exit (1);
    }

  if (!Dflag && !Eflag && !Sflag && !aflag && !lflag)
    show_signon ();

  if (hash_init (dname) < 0)
    {
      (void) fprintf (stderr, "hash_init failed\n");
      exit (1);
    }

  if (hflag)
    {
      prhashchain ();
      exit (0);
    }

  if (Dflag)
    {
      hash_awrite (stdout);
      exit (0);
    }

  if (Eflag)
    {
      hash_ewrite (stdout);
      exit (0);
    }

  if (pflag == 0)
    {
      if ((p = (char *) getenv ("HOME")) != NULL)
	{
	  privname = (char *) xmalloc ((unsigned) (strlen (p) + 50));
	  (void) strcpy (privname, p);
	  (void) strcat (privname, "/ispell.words");
	}
    }

  if (privname)
    (void) p_load (privname, 1);

  if (Sflag)
    {
      submode ();
    }
  else if (aflag)
    {
      askmode (0);
    }
  else if (uflag)
    {
      spellmode (argc, argv, optind);
      exit (0);
    }
  else if (lflag)
    {
      checkfile (stdin, (FILE *) NULL, (long) 0);
      exit (0);
    }
  else
    {
      /* normal interactive mode */
      if (optind == argc)
	{
	  askmode (1);
	  exit (0);
	}
      signal (SIGINT, (RETSIGTYPE (*)()) intr);
      terminit ();
      while (optind < argc)
	dofile (argv[optind++]);

      if (interaction_flag == 0)
	printf ("No errors detected\r\n");
    }

  if (privname)
    (void) p_dump (privname);

  done ();
  exit (0);
}

extern struct sp_corrections corrections;
static char stdoutbuf[BUFSIZ];

void
submode (NOARGS)
{
  char buf[BUFSIZ];
  int i;
  int c;
  int len;

  signal (SIGINT, SIG_IGN);
  setbuf (stdin, (char *) NULL);
  setbuf (stdout, stdoutbuf);
  (void) printf ("(1 \"%s\")", VERSION_STRING);

  while (1)
    {
    again:
      intr_typed = 0;
      putchar ('=');
      fflush (stdout);

      if (fgets (buf, sizeof buf, stdin) == NULL)
	break;
      len = strlen (buf);
      if (len && buf[len - 1] == '\n')
	{
	  buf[--len] = 0;
	}
      else
	{
	  /* line was too long to hold newline -
			 * just return an error without otherwise
			 * looking at it
			 */
	  while (1)
	    {
	      if ((c = getchar ()) == EOF)
		return;
	      if (c == '\n')
		{
		  (void) printf ("nil\n");
		  goto again;
		}
	    }
	}
      if (buf[0] == 0)
	continue;

      if (buf[0] == ':')
	{
	  if (subcmd (buf) < 0)
	    (void) printf ("nil\n");
	  else
	    (void) printf ("t\n");
	  continue;
	}
      if (good (buf, strlen (buf), 0))
	{
	  (void) printf ("t\n");
	}
      else
	{
	  makepossibilities (buf);
	  if (corrections.posbuf[0][0])
	    {
	      (void) printf ("(");
	      for (i = 0; i < 10; i++)
		{
		  if (corrections.posbuf[i][0] == 0)
		    break;
		  (void) printf ("\"%s\" ",
				 corrections.posbuf[i]);
		}
	      (void) printf (")\n");
	    }
	  else
	    {
	      (void) printf ("nil\n");
	    }
	}
    }
}

struct cmd
{
  char *name;
#ifdef __STDC__
  int (*func) (char *);
#else
  int (*func) ();
#endif
};

extern struct cmd subcmds[];

int
subcmd (buf)
  char *buf;
{
  char *p;
  struct cmd *cp;

  buf++;			/* skip colon */
  for (p = buf; *p && !isspace (*p); p++)
    ;
  if (*p)
    *p++ = 0;

  for (cp = subcmds; cp->name; cp++)
    if (strcmp (cp->name, buf) == 0)
      break;
  if (cp->name == NULL)
    return (-1);

  return ((*cp->func) (p));
}


int
cmd_insert (arg)
     char *arg;
{
  return (p_enter (arg, 1, 1));
}


int
cmd_accept (arg)
  char *arg;
{
  return (p_enter (arg, 1, 0));
}


int
cmd_delete (arg)
  char *arg;
{
  return (p_delete (arg));
}


int
cmd_dump (arg)
  char *arg;
{
  if (privname)
    return (p_dump (privname));
  else
    return (-1);
}

/* ARGSUSED */
int
cmd_reload (arg)
  char *arg;
{
  return p_reload ();
}


int
cmd_file (arg)
  char *arg;
{
  FILE *f;
  long start, end;
  char *p;

  for (p = arg; *p && !isspace (*p); p++)
    ;
  if (*p)
    *p++ = 0;

  start = 0;
  end = 0;

  sscanf (p, "%ld %ld", &start, &end);

  if ((f = fopen (arg, "r")) == NULL)
    return -1;

  if (fseek (f, start, 0) < 0)
    {
      (void) fclose (f);
      return (-1);
    }

  signal (SIGINT, (RETSIGTYPE (*)()) intr);
  checkfile (f, (FILE *) NULL, end);
  signal (SIGINT, SIG_IGN);

  (void) fclose (f);
  if (intr_typed)
    {
      intr_typed = 0;
      return (-1);
    }
  else
    {
      return (0);
    }
}

/* ARGSUSED */
int
cmd_tex (arg)
  char *arg;
{
  formatter = formatter_tex;
  return (0);
}

/* ARGSUSED */
int
cmd_troff (arg)
  char *arg;
{
  formatter = formatter_troff;
  return (0);
}

/* ARGUSED */
int
cmd_generic (arg)
  char *arg;
{
  formatter = formatter_generic;
  return (0);
}


void
spellmode (ac, av, ind)
  char **av;
  int ac, ind;
{
  FILE *f;

  sortf = (FILE *) popen ("sort", "w");
  if (sortf == NULL)
    {
      (void) fprintf (stderr, "can't exec 'sort' program\n");
      exit (1);
    }
  if (ac == ind)
    {
      checkfile (stdin, (FILE *) NULL, (long) 0);
    }
  else
    {
      while (ind < ac)
	{
	  f = fopen (av[ind], "r");
	  if (f == NULL)
	    {
	      (void) fprintf (stderr, "can't open %s\n", av[ind]);
	    }
	  else
	    {
	      checkfile (f, (FILE *) NULL, (long) 0);
	      (void) fclose (f);
	    }
	  ind++;
	}
    }
  (void) pclose (sortf);	/* needed to wait for sort to finish */
}


struct cmd subcmds[] =
{
  {"insert", cmd_insert},
  {"accept", cmd_accept},
  {"delete", cmd_delete},
  {"dump", cmd_dump},
  {"reload", cmd_reload},
  {"file", cmd_file},
  {"tex", cmd_tex},
  {"troff", cmd_troff},
  {"generic", cmd_generic},
  {NULL}
};

#define p(str) printf("%s\n", str)

static void
show_signon ()
{
  printf ("%s.\n", VERSION_STRING);
  p ("Copyright (C) 1990, 1993 Free Software Foundation, Inc.");
  p ("Ispell comes with ABSOLUTELY NO WARRANTY; for details run \"ispell -W\"");
  p ("This is free software, and you are welcome to redistribute it");
  p ("under certain conditions; run \"ispell -C\" for details.");
}


static void
show_copying ()
{
  p ("		    GNU GENERAL PUBLIC LICENSE");
  p ("		       Version 2, June 1991");
  p ("");
  p (" Copyright (C) 1989, 1991 Free Software Foundation, Inc.");
  p ("                          675 Mass Ave, Cambridge, MA 02139, USA");
  p (" Everyone is permitted to copy and distribute verbatim copies");
  p (" of this license document, but changing it is not allowed.");
  p ("");
  p ("			    Preamble");
  p ("");
  p ("  The licenses for most software are designed to take away your");
  p ("freedom to share and change it.  By contrast, the GNU General Public");
  p ("License is intended to guarantee your freedom to share and change free");
  p ("software--to make sure the software is free for all its users.  This");
  p ("General Public License applies to most of the Free Software");
  p ("Foundation's software and to any other program whose authors commit to");
  p ("using it.  (Some other Free Software Foundation software is covered by");
  p ("the GNU Library General Public License instead.)  You can apply it to");
  p ("your programs, too.");
  p ("");
  p ("  When we speak of free software, we are referring to freedom, not");
  p ("price.  Our General Public Licenses are designed to make sure that you");
  p ("have the freedom to distribute copies of free software (and charge for");
  p ("this service if you wish), that you receive source code or can get it");
  p ("if you want it, that you can change the software or use pieces of it");
  p ("in new free programs; and that you know you can do these things.");
  p ("");
  p ("  To protect your rights, we need to make restrictions that forbid");
  p ("anyone to deny you these rights or to ask you to surrender the rights.");
  p ("These restrictions translate to certain responsibilities for you if you");
  p ("distribute copies of the software, or if you modify it.");
  p ("");
  p ("  For example, if you distribute copies of such a program, whether");
  p ("gratis or for a fee, you must give the recipients all the rights that");
  p ("you have.  You must make sure that they, too, receive or can get the");
  p ("source code.  And you must show them these terms so they know their");
  p ("rights.");
  p ("");
  p ("  We protect your rights with two steps: (1) copyright the software, and");
  p ("(2) offer you this license which gives you legal permission to copy,");
  p ("distribute and/or modify the software.");
  p ("");
  p ("  Also, for each author's protection and ours, we want to make certain");
  p ("that everyone understands that there is no warranty for this free");
  p ("software.  If the software is modified by someone else and passed on, we");
  p ("want its recipients to know that what they have is not the original, so");
  p ("that any problems introduced by others will not reflect on the original");
  p ("authors' reputations.");
  p ("");
  p ("  Finally, any free program is threatened constantly by software");
  p ("patents.  We wish to avoid the danger that redistributors of a free");
  p ("program will individually obtain patent licenses, in effect making the");
  p ("program proprietary.  To prevent this, we have made it clear that any");
  p ("patent must be licensed for everyone's free use or not licensed at all.");
  p ("");
  p ("  The precise terms and conditions for copying, distribution and");
  p ("modification follow.");
  p ("");
  p ("		    GNU GENERAL PUBLIC LICENSE");
  p ("   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION");
  p ("");
  p ("  0. This License applies to any program or other work which contains");
  p ("a notice placed by the copyright holder saying it may be distributed");
  p ("under the terms of this General Public License.  The \"Program\", below,");
  p ("refers to any such program or work, and a \"work based on the Program\"");
  p ("means either the Program or any derivative work under copyright law:");
  p ("that is to say, a work containing the Program or a portion of it,");
  p ("either verbatim or with modifications and/or translated into another");
  p ("language.  (Hereinafter, translation is included without limitation in");
  p ("the term \"modification\".)  Each licensee is addressed as \"you\".");
  p ("");
  p ("Activities other than copying, distribution and modification are not");
  p ("covered by this License; they are outside its scope.  The act of");
  p ("running the Program is not restricted, and the output from the Program");
  p ("is covered only if its contents constitute a work based on the");
  p ("Program (independent of having been made by running the Program).");
  p ("Whether that is true depends on what the Program does.");
  p ("");
  p ("  1. You may copy and distribute verbatim copies of the Program's");
  p ("source code as you receive it, in any medium, provided that you");
  p ("conspicuously and appropriately publish on each copy an appropriate");
  p ("copyright notice and disclaimer of warranty; keep intact all the");
  p ("notices that refer to this License and to the absence of any warranty;");
  p ("and give any other recipients of the Program a copy of this License");
  p ("along with the Program.");
  p ("");
  p ("You may charge a fee for the physical act of transferring a copy, and");
  p ("you may at your option offer warranty protection in exchange for a fee.");
  p ("");
  p ("  2. You may modify your copy or copies of the Program or any portion");
  p ("of it, thus forming a work based on the Program, and copy and");
  p ("distribute such modifications or work under the terms of Section 1");
  p ("above, provided that you also meet all of these conditions:");
  p ("");
  p ("    a) You must cause the modified files to carry prominent notices");
  p ("    stating that you changed the files and the date of any change.");
  p ("");
  p ("    b) You must cause any work that you distribute or publish, that in");
  p ("    whole or in part contains or is derived from the Program or any");
  p ("    part thereof, to be licensed as a whole at no charge to all third");
  p ("    parties under the terms of this License.");
  p ("");
  p ("    c) If the modified program normally reads commands interactively");
  p ("    when run, you must cause it, when started running for such");
  p ("    interactive use in the most ordinary way, to print or display an");
  p ("    announcement including an appropriate copyright notice and a");
  p ("    notice that there is no warranty (or else, saying that you provide");
  p ("    a warranty) and that users may redistribute the program under");
  p ("    these conditions, and telling the user how to view a copy of this");
  p ("    License.  (Exception: if the Program itself is interactive but");
  p ("    does not normally print such an announcement, your work based on");
  p ("    the Program is not required to print an announcement.)");
  p ("");
  p ("These requirements apply to the modified work as a whole.  If");
  p ("identifiable sections of that work are not derived from the Program,");
  p ("and can be reasonably considered independent and separate works in");
  p ("themselves, then this License, and its terms, do not apply to those");
  p ("sections when you distribute them as separate works.  But when you");
  p ("distribute the same sections as part of a whole which is a work based");
  p ("on the Program, the distribution of the whole must be on the terms of");
  p ("this License, whose permissions for other licensees extend to the");
  p ("entire whole, and thus to each and every part regardless of who wrote it.");
  p ("");
  p ("Thus, it is not the intent of this section to claim rights or contest");
  p ("your rights to work written entirely by you; rather, the intent is to");
  p ("exercise the right to control the distribution of derivative or");
  p ("collective works based on the Program.");
  p ("");
  p ("In addition, mere aggregation of another work not based on the Program");
  p ("with the Program (or with a work based on the Program) on a volume of");
  p ("a storage or distribution medium does not bring the other work under");
  p ("the scope of this License.");
  p ("");
  p ("  3. You may copy and distribute the Program (or a work based on it,");
  p ("under Section 2) in object code or executable form under the terms of");
  p ("Sections 1 and 2 above provided that you also do one of the following:");
  p ("");
  p ("    a) Accompany it with the complete corresponding machine-readable");
  p ("    source code, which must be distributed under the terms of Sections");
  p ("    1 and 2 above on a medium customarily used for software interchange; or,");
  p ("");
  p ("    b) Accompany it with a written offer, valid for at least three");
  p ("    years, to give any third party, for a charge no more than your");
  p ("    cost of physically performing source distribution, a complete");
  p ("    machine-readable copy of the corresponding source code, to be");
  p ("    distributed under the terms of Sections 1 and 2 above on a medium");
  p ("    customarily used for software interchange; or,");
  p ("");
  p ("    c) Accompany it with the information you received as to the offer");
  p ("    to distribute corresponding source code.  (This alternative is");
  p ("    allowed only for noncommercial distribution and only if you");
  p ("    received the program in object code or executable form with such");
  p ("    an offer, in accord with Subsection b above.)");
  p ("");
  p ("The source code for a work means the preferred form of the work for");
  p ("making modifications to it.  For an executable work, complete source");
  p ("code means all the source code for all modules it contains, plus any");
  p ("associated interface definition files, plus the scripts used to");
  p ("control compilation and installation of the executable.  However, as a");
  p ("special exception, the source code distributed need not include");
  p ("anything that is normally distributed (in either source or binary");
  p ("form) with the major components (compiler, kernel, and so on) of the");
  p ("operating system on which the executable runs, unless that component");
  p ("itself accompanies the executable.");
  p ("");
  p ("If distribution of executable or object code is made by offering");
  p ("access to copy from a designated place, then offering equivalent");
  p ("access to copy the source code from the same place counts as");
  p ("distribution of the source code, even though third parties are not");
  p ("compelled to copy the source along with the object code.");
  p ("");
  p ("  4. You may not copy, modify, sublicense, or distribute the Program");
  p ("except as expressly provided under this License.  Any attempt");
  p ("otherwise to copy, modify, sublicense or distribute the Program is");
  p ("void, and will automatically terminate your rights under this License.");
  p ("However, parties who have received copies, or rights, from you under");
  p ("this License will not have their licenses terminated so long as such");
  p ("parties remain in full compliance.");
  p ("");
  p ("  5. You are not required to accept this License, since you have not");
  p ("signed it.  However, nothing else grants you permission to modify or");
  p ("distribute the Program or its derivative works.  These actions are");
  p ("prohibited by law if you do not accept this License.  Therefore, by");
  p ("modifying or distributing the Program (or any work based on the");
  p ("Program), you indicate your acceptance of this License to do so, and");
  p ("all its terms and conditions for copying, distributing or modifying");
  p ("the Program or works based on it.");
  p ("");
  p ("  6. Each time you redistribute the Program (or any work based on the");
  p ("Program), the recipient automatically receives a license from the");
  p ("original licensor to copy, distribute or modify the Program subject to");
  p ("these terms and conditions.  You may not impose any further");
  p ("restrictions on the recipients' exercise of the rights granted herein.");
  p ("You are not responsible for enforcing compliance by third parties to");
  p ("this License.");
  p ("");
  p ("  7. If, as a consequence of a court judgment or allegation of patent");
  p ("infringement or for any other reason (not limited to patent issues),");
  p ("conditions are imposed on you (whether by court order, agreement or");
  p ("otherwise) that contradict the conditions of this License, they do not");
  p ("excuse you from the conditions of this License.  If you cannot");
  p ("distribute so as to satisfy simultaneously your obligations under this");
  p ("License and any other pertinent obligations, then as a consequence you");
  p ("may not distribute the Program at all.  For example, if a patent");
  p ("license would not permit royalty-free redistribution of the Program by");
  p ("all those who receive copies directly or indirectly through you, then");
  p ("the only way you could satisfy both it and this License would be to");
  p ("refrain entirely from distribution of the Program.");
  p ("");
  p ("If any portion of this section is held invalid or unenforceable under");
  p ("any particular circumstance, the balance of the section is intended to");
  p ("apply and the section as a whole is intended to apply in other");
  p ("circumstances.");
  p ("");
  p ("It is not the purpose of this section to induce you to infringe any");
  p ("patents or other property right claims or to contest validity of any");
  p ("such claims; this section has the sole purpose of protecting the");
  p ("integrity of the free software distribution system, which is");
  p ("implemented by public license practices.  Many people have made");
  p ("generous contributions to the wide range of software distributed");
  p ("through that system in reliance on consistent application of that");
  p ("system; it is up to the author/donor to decide if he or she is willing");
  p ("to distribute software through any other system and a licensee cannot");
  p ("impose that choice.");
  p ("");
  p ("This section is intended to make thoroughly clear what is believed to");
  p ("be a consequence of the rest of this License.");
  p ("");
  p ("  8. If the distribution and/or use of the Program is restricted in");
  p ("certain countries either by patents or by copyrighted interfaces, the");
  p ("original copyright holder who places the Program under this License");
  p ("may add an explicit geographical distribution limitation excluding");
  p ("those countries, so that distribution is permitted only in or among");
  p ("countries not thus excluded.  In such case, this License incorporates");
  p ("the limitation as if written in the body of this License.");
  p ("");
  p ("  9. The Free Software Foundation may publish revised and/or new versions");
  p ("of the General Public License from time to time.  Such new versions will");
  p ("be similar in spirit to the present version, but may differ in detail to");
  p ("address new problems or concerns.");
  p ("");
  p ("Each version is given a distinguishing version number.  If the Program");
  p ("specifies a version number of this License which applies to it and \"any");
  p ("later version\", you have the option of following the terms and conditions");
  p ("either of that version or of any later version published by the Free");
  p ("Software Foundation.  If the Program does not specify a version number of");
  p ("this License, you may choose any version ever published by the Free Software");
  p ("Foundation.");
  p ("");
  p ("  10. If you wish to incorporate parts of the Program into other free");
  p ("programs whose distribution conditions are different, write to the author");
  p ("to ask for permission.  For software which is copyrighted by the Free");
  p ("Software Foundation, write to the Free Software Foundation; we sometimes");
  p ("make exceptions for this.  Our decision will be guided by the two goals");
  p ("of preserving the free status of all derivatives of our free software and");
  p ("of promoting the sharing and reuse of software generally.");
  p ("");
}


static void
show_warranty ()
{
  p ("			    NO WARRANTY");
  p ("");
  p ("  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY");
  p ("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN");
  p ("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES");
  p ("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED");
  p ("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF");
  p ("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS");
  p ("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE");
  p ("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,");
  p ("REPAIR OR CORRECTION.");
  p ("");
  p ("  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING");
  p ("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR");
  p ("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,");
  p ("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING");
  p ("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED");
  p ("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY");
  p ("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER");
  p ("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE");
  p ("POSSIBILITY OF SUCH DAMAGES.");
  p ("");
}
