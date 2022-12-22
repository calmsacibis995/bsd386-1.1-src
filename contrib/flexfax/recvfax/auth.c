/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/recvfax/auth.c,v 1.1.1.1 1994/01/14 23:10:39 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */
#include "defs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

static char*
topdomain(char* h)
{
    register char* p;
    char* maybe = NULL;
    int dots = 0;

    for (p = h + strlen(h); p >= h; p--)
	if (*p == '.') {
	    if (++dots == 2)
		return (p);
	    maybe = p;
	}
    return (maybe);
}

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
static int
local_domain(char* h)
{
    char localhost[80];
    char* p1;
    char* p2;

    localhost[0] = 0;
    (void) gethostname(localhost, sizeof (localhost));
    p1 = topdomain(localhost);
    p2 = topdomain(h);
    return (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2));
}

extern int rexMatch(const char* pat,
		const char* dotform, int dotlen,
		const char* hostform, int hostlen);

void
checkPermission()
{
    static int alreadyChecked = 0;
    char dotform[1024], hostform[1024];
    int dotlen, hostlen;
    char* dotaddr;
    struct sockaddr_in sin;
    int sinlen;
    FILE* db;

    if (alreadyChecked)
	return;
    alreadyChecked = 1;
    sinlen = sizeof (sin);
    if (getpeername(fileno(stdin), (struct sockaddr*)&sin, &sinlen) < 0) {
	syslog(LOG_ERR, "getpeername: %m");
	sendError("Can not get your network address.");
	done(-1, "EXIT");
    }
    dotaddr = inet_ntoa(sin.sin_addr);
    db = fopen(FAX_PERMFILE, "r");
    if (db) {
	struct hostent* hp;
	char* hostname = NULL;
	char line[1024];

	hp = gethostbyaddr(&sin.sin_addr, sizeof (sin.sin_addr),
		sin.sin_family);
	if (hp) {
	    /*
	     * If name returned by gethostbyaddr is in our domain,
	     * attempt to verify that we haven't been fooled by someone
	     * in a remote net; look up the name and check that this
	     * address corresponds to the name.
	     */
	    if (local_domain(hp->h_name)) {
		strncpy(line, hp->h_name, sizeof (line) - 1);
		line[sizeof (line) - 1] = '\0';
		hp = gethostbyname(line);
		if (hp) {
		    for (; hp->h_addr_list[0] != NULL; hp->h_addr_list++) {
			if (memcmp(hp->h_addr_list[0], (caddr_t) &sin.sin_addr,
			  hp->h_length) == 0) {
			    hostname = hp->h_name;	/* accept name */
			    break;
			}
		    }
		    if (hostname == NULL)
			sendAndLogError(
		    "Your host address \"%s\" is not listed for host \"%s\".",
			    dotaddr, hp->h_name);
		} else
		    sendAndLogError("Could not find the address for \"%s\".",
		       line);
	    } else
		hostname = hp->h_name;
	} else
	    sendAndLogError(
		"Can not map your network address (\"%s\") to a hostname",
		dotaddr);
	/*
	 * Now check the host name/address against
	 * the list of hosts that are permitted to
	 * submit jobs.
	 */
	strcpy(dotform, userID);
	strcat(dotform, "@");
	strcat(dotform, dotaddr);
	dotlen = strlen(dotform);
	if (hostname != NULL) {
	    strcpy(hostform, userID);
	    strcat(hostform, "@");
	    strcat(hostform, hostname);
	    hostlen = strlen(hostform);
	} else
	    hostlen = 0;
	while (fgets(line, sizeof (line)-1, db)) {
	    char* cp = cp = strchr(line, '#');
	    if (cp || (cp = strchr(line, '\n')))
		*cp = '\0';
	    /* trim off trailing white space */
	    for (cp = strchr(line, '\0'); cp > line; cp--)
		if (!isspace(cp[-1]))
		    break;
	    *cp = '\0';
	    if (line[0] == '!') {
		/*
		 * "!" in the first column means, disallow on match.
		 */
		if (rexMatch(line+1, dotform, dotlen, hostform, hostlen))
		    break;
	    } else {
		/*
		 * Hack for backwards compatibility; take pure host
		 * specification and form the regex ".*@<host>" to
		 * match against all users from that host.
		 */
		if (strchr(line+0, '@') == NULL) {
		    char line1[1024];
		    strcpy(line1, ".*@");
		    strcat(line1, line+0);
		    if (rexMatch(line1, dotform, dotlen, hostform, hostlen))
			return;
		} else if (rexMatch(line+0, dotform, dotlen, hostform, hostlen))
		    return;
	    }
	}
	fclose(db);
    } else
	sendError("The server does not have a permissions file.");
    syslog(LOG_ERR, "%s: Service refused", hostlen == 0 ? dotform : hostform);
    sendError("You do not have permission to %s.",
	"use the fax server from this host");
    done(-1, "EXIT");
}
