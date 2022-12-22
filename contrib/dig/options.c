
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 ***************************************************************************
 *
 *  SetOption -- 
 *
 *	This routine is used to change the state information
 *	that affect the lookups. The command format is
 *	   set keyword[=value]
 *	Most keywords can be abbreviated. Parsing is very simplistic--
 *	A value must not be separated from its keyword by white space.
 *
 *      Andrew Cherenson        CS298-26  Fall 1985
 *
 ***************************************************************************
 */

/*
** Modified for 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

int
SetOption(string)
    char *string;
{
    char 	option[NAME_LEN];
    char 	type[NAME_LEN];
    char 	*ptr;
    int 	i;

    i = sscanf(string, " %s", option);
    if (i != 1) {
	fprintf(stderr, ";*** Invalid option: %s\n",  option);
	return(ERROR);
    } 
   
    if (strncmp(option, "aa", 2) == 0) {	/* aaonly */
	    _res.options |= RES_AAONLY;
	} else if (strncmp(option, "noaa", 4) == 0) {
	    _res.options &= ~RES_AAONLY;
	} else if (strncmp(option, "deb", 3) == 0) {	/* debug */
	    _res.options |= RES_DEBUG;
	} else if (strncmp(option, "nodeb", 5) == 0) {
	    _res.options &= ~(RES_DEBUG | RES_DEBUG2);
	} else if (strncmp(option, "ko", 2) == 0) {	/* keepopen */
	    _res.options |= (RES_STAYOPEN | RES_USEVC);
	} else if (strncmp(option, "noko", 4) == 0) {
	    _res.options &= ~RES_STAYOPEN;
	} else if (strncmp(option, "d2", 2) == 0) {	/* d2 (more debug) */
	    _res.options |= (RES_DEBUG | RES_DEBUG2);
	} else if (strncmp(option, "nod2", 4) == 0) {
	    _res.options &= ~RES_DEBUG2;
	} else if (strncmp(option, "def", 3) == 0) {	/* defname */
	    _res.options |= RES_DEFNAMES;
	} else if (strncmp(option, "nodef", 5) == 0) {
	    _res.options &= ~RES_DEFNAMES;
	} else if (strncmp(option, "sea", 3) == 0) {	/* search list */
	    _res.options |= RES_DNSRCH;
	} else if (strncmp(option, "nosea", 5) == 0) {
	    _res.options &= ~RES_DNSRCH;
	} else if (strncmp(option, "do", 2) == 0) {	/* domain */
	    ptr = index(option, '=');
	    if (ptr != NULL) {
		sscanf(++ptr, "%s", _res.defdname);
		res_re_init();
	    }
	  } else if (strncmp(option, "ti", 2) == 0) {      /* timeout */
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      sscanf(++ptr, "%d", &_res.retrans);
	    }

	  } else if (strncmp(option, "ret", 3) == 0) {    /* retry */
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      sscanf(++ptr, "%d", &_res.retry);
	    }

	} else if (strncmp(option, "i", 1) == 0) {	/* ignore */
	    _res.options |= RES_IGNTC;
	} else if (strncmp(option, "noi", 3) == 0) {
	    _res.options &= ~RES_IGNTC;
	} else if (strncmp(option, "pr", 2) == 0) {	/* primary */
	    _res.options |= RES_PRIMARY;
	} else if (strncmp(option, "nop", 3) == 0) {
	    _res.options &= ~RES_PRIMARY;
	} else if (strncmp(option, "rec", 3) == 0) {	/* recurse */
	    _res.options |= RES_RECURSE;
	} else if (strncmp(option, "norec", 5) == 0) {
	    _res.options &= ~RES_RECURSE;
	} else if (strncmp(option, "v", 1) == 0) {	/* vc */
	    _res.options |= RES_USEVC;
	} else if (strncmp(option, "nov", 3) == 0) {
	    _res.options &= ~RES_USEVC;
	} else if (strncmp(option, "pfset", 5) == 0) {
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      pfcode = xstrtonum(++ptr);
	    }
	} else if (strncmp(option, "pfand", 5) == 0) {
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      pfcode = pfcode & xstrtonum(++ptr);
	    }
	} else if (strncmp(option, "pfor", 4) == 0) {
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      pfcode = pfcode | xstrtonum(++ptr);
	    }
	} else if (strncmp(option, "pfmin", 5) == 0) {
	      pfcode = PRF_MIN;
	} else if (strncmp(option, "pfdef", 5) == 0) {
	      pfcode = PRF_DEF;
	} else if (strncmp(option, "an", 2) == 0) {  /* answer section */
	      pfcode |= PRF_ANS;
	} else if (strncmp(option, "noan", 4) == 0) {
	      pfcode &= ~PRF_ANS;
	} else if (strncmp(option, "qu", 2) == 0) {  /* question section */
	      pfcode |= PRF_QUES;
	} else if (strncmp(option, "noqu", 4) == 0) {  
	      pfcode &= ~PRF_QUES;
	} else if (strncmp(option, "au", 2) == 0) {  /* authority section */
	      pfcode |= PRF_AUTH;
	} else if (strncmp(option, "noau", 4) == 0) {  
	      pfcode &= ~PRF_AUTH;
	} else if (strncmp(option, "ad", 2) == 0) {  /* addition section */
	      pfcode |= PRF_ADD;
	} else if (strncmp(option, "noad", 4) == 0) {  
	      pfcode &= ~PRF_ADD;
	} else if (strncmp(option, "tt", 2) == 0) {  /* TTL & ID */
	      pfcode |= PRF_TTLID;
	} else if (strncmp(option, "nott", 4) == 0) {  
	      pfcode &= ~PRF_TTLID;
	} else if (strncmp(option, "he", 2) == 0) {  /* head flags stats */
	      pfcode |= PRF_HEAD2;
	} else if (strncmp(option, "nohe", 4) == 0) {  
	      pfcode &= ~PRF_HEAD2;
	} else if (strncmp(option, "H", 1) == 0) {  /* header all */
	      pfcode |= PRF_HEADX;
	} else if (strncmp(option, "noH", 3) == 0) {  
	      pfcode &= ~(PRF_HEADX);
	} else if (strncmp(option, "qr", 2) == 0) {  /* query */
	      pfcode |= PRF_QUERY;
	} else if (strncmp(option, "noqr", 4) == 0) {  
	      pfcode &= ~PRF_QUERY;
	} else if (strncmp(option, "rep", 3) == 0) {  /* reply */
	      pfcode |= PRF_REPLY;
	} else if (strncmp(option, "norep", 5) == 0) {  
	      pfcode &= ~PRF_REPLY;
	} else if (strncmp(option, "cm", 2) == 0) {  /* command line */
	      pfcode |= PRF_CMD;
	} else if (strncmp(option, "nocm", 4) == 0) {  
	      pfcode &= ~PRF_CMD;
	} else if (strncmp(option, "cl", 2) == 0) {  /* class mnemonic */
	      pfcode |= PRF_CLASS;
	} else if (strncmp(option, "nocl", 4) == 0) {  
	      pfcode &= ~PRF_CLASS;
	} else if (strncmp(option, "st", 2) == 0) {  /* stats*/
	      pfcode |= PRF_STATS;
	} else if (strncmp(option, "nost", 4) == 0) {  
	      pfcode &= ~PRF_STATS;
	} else if (strncmp(option, "sor", 3) == 0) {  /* sort */
	      pfcode |= PRF_SORT;
	} else if (strncmp(option, "nosor", 5) == 0) {  
	      pfcode &= ~PRF_SORT;
	} else {
	    fprintf(stderr, "; *** Invalid option: %s\n",  option);
	    return(ERROR);
	  }
    return(SUCCESS);
}



/*
 * Fake a reinitialization when the domain is changed.
 */
res_re_init()
{
    register char *cp, **pp;
    int n;

    /* find components of local domain that might be searched */
    pp = _res.dnsrch;
    *pp++ = _res.defdname;
    for (cp = _res.defdname, n = 0; *cp; cp++)
	if (*cp == '.')
	    n++;
    cp = _res.defdname;
    for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch + MAXDNSRCH; n--) {
	cp = index(cp, '.');
	*pp++ = ++cp;
    }
    *pp = 0;
    _res.options |= RES_INIT;
}



/*
** Written for 'dig' version 1.0 at University of Southern
** California Information Sciences Institute (USC-ISI). 3/27/89
*/
/*
 * convert char string (decimal, octal, or hex) to integer
 */
int xstrtonum(p)
char *p;
{
int v = 0;
int i;
int b = 10;
int flag = 0;
while (*p != 0) {
  if (!flag++)
    if (*p == '0') {
      b = 8; p++;
      continue;
    }
  if (isupper(*p))
      *p=tolower(*p);
  if (*p == 'x') {
    b = 16; p++;
    continue;
  }
  if (isdigit(*p)) {
    i = *p - '0';
  } else if (isxdigit(*p)) {
    i = *p - 'a' + 10;
  } else {
    fprintf(stderr,"; *** Bad char in numeric string -- ignored\n");
    i = -1;
  }
  if (i >= b) {
    fprintf(stderr,"; *** Bad char in numeric string -- ignored\n");
    i = -1;
  }
  if (i >= 0)
    v = v * b + i;
p++;
}
return(v);
}

