/************************************************************************
 *   IRC - Internet Relay Chat, ircd/hash.c
 *   Copyright (C) 1991 Darren Reed
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
#ifndef lint
static char sccsid[] = "@(#)hash.c	2.10 03 Jul 1993 (C) 1991 Darren Reed";
#endif

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "hash.h"
#include "h.h"

#ifdef	DEBUGMODE
static	aHashEntry	*clientTable = NULL;
static	aHashEntry	*channelTable = NULL;
static	int	clhits, clmiss;
static	int	chhits, chmiss;
int	HASHSIZE = 2003;
int	CHANNELHASHSIZE = 607;
#else
static	aHashEntry	clientTable[HASHSIZE];
static	aHashEntry	channelTable[CHANNELHASHSIZE];
#endif

static	int	hash_mult[] = { 173, 179, 181, 191, 193, 197,
				199, 211, 223, 227, 229, 233,
				239, 241, 251, 257, 263, 269,
				271, 277, 281, 293, 307, 311,
				401, 409, 419, 421, 431, 433,
				439, 443, 449, 457, 461, 463
				};
/*
 * Hashing.
 *
 *   The server uses a chained hash table to provide quick and efficient
 * hash table mantainence (providing the hash function works evenly over
 * the input range).  The hash table is thus not susceptible to problems
 * of filling all the buckets or the need to rehash.
 *    It is expected that the hash table would look somehting like this
 * during use:
 *                   +-----+    +-----+    +-----+   +-----+
 *                ---| 224 |----| 225 |----| 226 |---| 227 |---
 *                   +-----+    +-----+    +-----+   +-----+
 *                      |          |          |
 *                   +-----+    +-----+    +-----+
 *                   |  A  |    |  C  |    |  D  |
 *                   +-----+    +-----+    +-----+
 *                      |
 *                   +-----+
 *                   |  B  |
 *                   +-----+
 *
 * A - GOPbot, B - chang, C - hanuaway, D - *.mu.OZ.AU
 *
 * The order shown above is just one instant of the server.  Each time a
 * lookup is made on an entry in the hash table and it is found, the entry
 * is moved to the top of the chain.
 */

/*
 * hash_nick_name
 *
 * this function must be *quick*.  Thus there should be no multiplication
 * or division or modulus in the inner loop.  subtraction and other bit
 * operations allowed.
 */
int	hash_nick_name(nname)
char	*nname;
{
	Reg1	u_char	*name = (u_char *)nname;
	Reg2	u_char	ch;
	Reg4	int	hash = 1, *tab;

	for (tab = hash_mult; (ch = *name); name++, tab++)
		hash += tolower(ch) + *tab + hash;
	if (hash < 0)
		hash = -hash;
	hash %= HASHSIZE;
	return (hash);
}

/*
 * hash_channel_name
 *
 * calculate a hash value on at most the first 30 characters of the channel
 * name. Most names are short than this or dissimilar in this range. There
 * is little or no point hashing on a full channel name which maybe 255 chars
 * long.
 */
int	hash_channel_name(hname)
char	*hname;
{
	Reg1	u_char	*name = (u_char *)hname;
	Reg2	u_char	ch;
	Reg3	int	i = 30;
	Reg4	int	hash = 5, *tab;

	for (tab = hash_mult; (ch = *name) && --i; name++, tab++)
		hash += tolower(ch) + *tab + hash + i + i;
	if (hash < 0)
		hash = -hash;
	hash %= CHANNELHASHSIZE;
	return (hash);
}

/*
 * clear_*_hash_table
 *
 * Nullify the hashtable and its contents so it is completely empty.
 */
void	clear_client_hash_table()
{
#ifdef	DEBUGMODE
	clhits = 0;
	clmiss = 0;
	if (!clientTable)
		clientTable = (aHashEntry *)MyMalloc(HASHSIZE *
						     sizeof(aHashEntry));
#endif

	bzero((char *)clientTable, sizeof(aHashEntry) * HASHSIZE);
}

void	clear_channel_hash_table()
{
#ifdef	DEBUGMODE
	chmiss = 0;
	chhits = 0;
	if (!channelTable)
		channelTable = (aHashEntry *)MyMalloc(CHANNELHASHSIZE *
						     sizeof(aHashEntry));
#endif
	bzero((char *)channelTable, sizeof(aHashEntry) * CHANNELHASHSIZE);
}

/*
 * add_to_client_hash_table
 */
int	add_to_client_hash_table(name, cptr)
char	*name;
aClient	*cptr;
{
	Reg1	int	hashv;

	hashv = hash_nick_name(name);
	cptr->hnext = (aClient *)clientTable[hashv].list;
	clientTable[hashv].list = (void *)cptr;
	clientTable[hashv].links++;
	clientTable[hashv].hits++;
	return 0;
}

/*
 * add_to_channel_hash_table
 */
int	add_to_channel_hash_table(name, chptr)
char	*name;
aChannel	*chptr;
{
	Reg1	int	hashv;

	hashv = hash_channel_name(name);
	chptr->hnextch = (aChannel *)channelTable[hashv].list;
	channelTable[hashv].list = (void *)chptr;
	channelTable[hashv].links++;
	channelTable[hashv].hits++;
	return 0;
}

/*
 * del_from_client_hash_table
 */
int	del_from_client_hash_table(name, cptr)
char	*name;
aClient	*cptr;
{
	Reg1	aClient	*tmp, *prev = NULL;
	Reg2	int	hashv;

	hashv = hash_nick_name(name);
	for (tmp = (aClient *)clientTable[hashv].list; tmp; tmp = tmp->hnext)
	    {
		if (tmp == cptr)
		    {
			if (prev)
				prev->hnext = tmp->hnext;
			else
				clientTable[hashv].list = (void *)tmp->hnext;
			tmp->hnext = NULL;
			if (clientTable[hashv].links > 0)
			    {
				clientTable[hashv].links--;
				return 1;
			    } 
			else
				/*
				 * Should never actually return from here and
				 * if we do it is an error/inconsistency in the
				 * hash table.
				 */
				return -1;
		    }
		prev = tmp;
	    }
	return 0;
}

/*
 * del_from_channel_hash_table
 */
int	del_from_channel_hash_table(name, chptr)
char	*name;
aChannel	*chptr;
{
	Reg1	aChannel	*tmp, *prev = NULL;
	Reg2	int	hashv;

	hashv = hash_channel_name(name);
	for (tmp = (aChannel *)channelTable[hashv].list; tmp;
	     tmp = tmp->hnextch)
	    {
		if (tmp == chptr)
		    {
			if (prev)
				prev->hnextch = tmp->hnextch;
			else
				channelTable[hashv].list=(void *)tmp->hnextch;
			tmp->hnextch = NULL;
			if (channelTable[hashv].links > 0)
			    {
				channelTable[hashv].links--;
				return 1;
			    }
			else
				return -1;
		    }
		prev = tmp;
	    }
	return 0;
}


/*
 * hash_find_client
 */
aClient	*hash_find_client(name, cptr)
char	*name;
aClient	*cptr;
{
	Reg1	aClient	*tmp;
	Reg2	aClient	*prv = NULL;
	Reg3	aHashEntry	*tmp3;
	int	hashv;

	hashv = hash_nick_name(name);
	tmp3 = &clientTable[hashv];

	/*
	 * Got the bucket, now search the chain.
	 */
	for (tmp = (aClient *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnext)
		if (mycmp(name, tmp->name) == 0)
			goto c_move_to_top;
#ifdef	DEBUGMODE
	clmiss++;
#endif
	return (cptr);
c_move_to_top:
#ifdef	DEBUGMODE
	clhits++;
#endif
	/*
	 * If the member of the hashtable we found isnt at the top of its
	 * chain, put it there.  This builds a most-frequently used order into
	 * the chains of the hash table, giving speadier lookups on those nicks
	 * which are being used currently.  This same block of code is also
	 * used for channels and servers for the same performance reasons.
	 */
	if (prv)
	    {
		aClient *tmp2;

		tmp2 = (aClient *)tmp3->list;
		tmp3->list = (void *)tmp;
		prv->hnext = tmp->hnext;
		tmp->hnext = tmp2;
	    }
	return (tmp);
}

/*
 * hash_find_nickserver
 */
aClient	*hash_find_nickserver(name, cptr)
char	*name;
aClient *cptr;
{
	Reg1	aClient	*tmp;
	Reg2	aClient	*prv = NULL;
	Reg3	aHashEntry	*tmp3;
	int	hashv;
	char	*serv;

	serv = index(name, '@');
	*serv++ = '\0';
	hashv = hash_nick_name(name);
	tmp3 = &clientTable[hashv];

	/*
	 * Got the bucket, now search the chain.
	 */
	for (tmp = (aClient *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnext)
		if (mycmp(name, tmp->name) == 0 && tmp->user &&
		    mycmp(serv, tmp->user->server) == 0)
			goto c_move_to_top;

#ifdef	DEBUGMODE
	clmiss++;
#endif
	*--serv = '\0';
	return (cptr);

c_move_to_top:
#ifdef	DEBUGMODE
	clhits++;
#endif
	/*
	 * If the member of the hashtable we found isnt at the top of its
	 * chain, put it there.  This builds a most-frequently used order into
	 * the chains of the hash table, giving speadier lookups on those nicks
	 * which are being used currently.  This same block of code is also
	 * used for channels and servers for the same performance reasons.
	 */
	if (prv)
	    {
		aClient *tmp2;

		tmp2 = (aClient *)tmp3->list;
		tmp3->list = (void *)tmp;
		prv->hnext = tmp->hnext;
		tmp->hnext = tmp2;
	    }
	*--serv = '\0';
	return (tmp);
}

/*
 * hash_find_server
 */
aClient	*hash_find_server(server, cptr)
char	*server;
aClient *cptr;
{
	Reg1	aClient	*tmp, *prv = NULL;
	Reg2	char	*t;
	Reg3	char	ch;
	aHashEntry	*tmp3;

	int hashv;

	hashv = hash_nick_name(server);
	tmp3 = &clientTable[hashv];

	for (tmp = (aClient *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnext)
	    {
		if (!IsServer(tmp) && !IsMe(tmp))
			continue;
		if (mycmp(server, tmp->name) == 0)
			goto s_move_to_top;
	    }
	t = ((char *)server + strlen(server));
	/*
	 * Whats happening in this next loop ? Well, it takes a name like
	 * foo.bar.edu and proceeds to earch for *.edu and then *.bar.edu.
	 * This is for checking full server names against masks although
	 * it isnt often done this way in lieu of using matches().
	 */
	for (;;)
	    {
		t--;
		for (; t > server; t--)
			if (*(t+1) == '.')
				break;
		if (*t == '*' || t == server)
			break;
		ch = *t;
		*t = '*';
		/*
	 	 * Dont need to check IsServer() here since nicknames cant
		 *have *'s in them anyway.
		 */
		if (((tmp = hash_find_client(t, cptr))) != cptr)
		    {
			*t = ch;
			return (tmp);
		    }
		*t = ch;
	    }
#ifdef	DEBUGMODE
	clmiss++;
#endif
	return (cptr);
s_move_to_top:
#ifdef	DEBUGMODE
	clhits++;
#endif
	if (prv)
	    {
		aClient *tmp2;

		tmp2 = (aClient *)tmp3->list;
		tmp3->list = (void *)tmp;
		prv->hnext = tmp->hnext;
		tmp->hnext = tmp2;
	    }
	return (tmp);
}

/*
 * hash_find_channel
 */
aChannel	*hash_find_channel(name, chptr)
char	*name;
aChannel *chptr;
{
	int	hashv;
	Reg1	aChannel	*tmp, *prv = NULL;
	aHashEntry	*tmp3;

	hashv = hash_channel_name(name);
	tmp3 = &channelTable[hashv];

	for (tmp = (aChannel *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnextch)
		if (mycmp(name, tmp->chname) == 0)
			goto c_move_to_top;
#ifdef	DEBUGMODE
	chmiss++;
#endif
	return chptr;
c_move_to_top:
#ifdef	DEBUGMODE
	chhits++;
#endif
	if (prv)
	    {
		register aChannel *tmp2;

		tmp2 = (aChannel *)tmp3->list;
		tmp3->list = (void *)tmp;
		prv->hnextch = tmp->hnextch;
		tmp->hnextch = tmp2;
	    }
	return (tmp);
}

/*
 * NOTE: this command is not supposed to be an offical part of the ircd
 *       protocol.  It is simply here to help debug and to monitor the
 *       performance of the hash functions and table, enabling a better
 *       algorithm to be sought if this one becomes troublesome.
 *       -avalon
 */

int	m_hash(cptr, sptr, parc, parv)
aClient	*cptr, *sptr;
int	parc;
char	*parv[];
{
#ifdef DEBUGMODE
	register	int	l, i;
	register	aHashEntry	*tab;
	int	deepest = 0, deeplink = 0, showlist = 0, tothits = 0;
	int	mosthit = 0, mosthits = 0, used = 0, used_now = 0, totlink = 0;
	int	link_pop[10], size = HASHSIZE;
	char	ch;
	aHashEntry	*table;

	if (parc > 1) {
		ch = *parv[1];
		if (islower(ch))
			table = clientTable;
		else {
			table = channelTable;
			size = CHANNELHASHSIZE;
		}
		if (ch == 'L' || ch == 'l')
			showlist = 1;
	} else {
		ch = '\0';
		table = clientTable;
	}

	for (i = 0; i < 10; i++)
		link_pop[i] = 0;
	for (i = 0; i < size; i++) {
		tab = &table[i];
		l = tab->links;
		if (showlist)
		    sendto_one(sptr,
			   "NOTICE %s :Hash Entry:%6d Hits:%7d Links:%6d",
			   parv[0], i, tab->hits, l);
		if (l > 0) {
			if (l < 10)
				link_pop[l]++;
			else
				link_pop[9]++;
			used_now++;
			totlink += l;
			if (l > deepest) {
				deepest = l;
				deeplink = i;
			}
		}
		else
			link_pop[0]++;
		l = tab->hits;
		if (l) {
			used++;
			tothits += l;
			if (l > mosthits) {
				mosthits = l;
				mosthit = i;
			}
		}
	}
	switch((int)ch)
	{
	case 'V' : case 'v' :
	    {
		register	aClient	*acptr;
		int	bad = 0, listlength = 0;

		for (acptr = client; acptr; acptr = acptr->next) {
			if (hash_find_client(acptr->name,acptr) != acptr) {
				if (ch == 'V')
				sendto_one(sptr, "NOTICE %s :Bad hash for %s",
					   parv[0], acptr->name);
				bad++;
			}
			listlength++;
		}
		sendto_one(sptr,"NOTICE %s :List Length: %d Bad Hashes: %d",
			   parv[0], listlength, bad);
	    }
	case 'P' : case 'p' :
		for (i = 0; i < 10; i++)
			sendto_one(sptr,"NOTICE %s :Entires with %d links : %d",
			parv[0], i, link_pop[i]);
		return (0);
	case 'r' :
	    {
		Reg1	aClient	*acptr;

		sendto_one(sptr,"NOTICE %s :Rehashing Client List.", parv[0]);
		clear_client_hash_table();
		for (acptr = client; acptr; acptr = acptr->next)
			(void)add_to_client_hash_table(acptr->name, acptr);
		break;
	    }
	case 'R' :
	    {
		Reg1	aChannel	*acptr;

		sendto_one(sptr,"NOTICE %s :Rehashing Channel List.", parv[0]);
		clear_channel_hash_table();
		for (acptr = channel; acptr; acptr = acptr->nextch)
			(void)add_to_channel_hash_table(acptr->chname, acptr);
		break;
	    }
	case 'H' :
		if (parc > 2)
			sendto_one(sptr,"NOTICE %s :%s hash to entry %d",
				   parv[0], parv[2],
				   hash_channel_name(parv[2]));
		return (0);
	case 'h' :
		if (parc > 2)
			sendto_one(sptr,"NOTICE %s :%s hash to entry %d",
				   parv[0], parv[2],
				   hash_nick_name(parv[2]));
		return (0);
	case 'n' :
	    {
		aClient	*tmp;
		int	max;

		if (parc <= 2)
			return (0);
		l = atoi(parv[2]) % HASHSIZE;
		if (parc > 3)
			max = atoi(parv[3]) % HASHSIZE;
		else
			max = l;
		for (;l <= max; l++)
			for (i = 0, tmp = (aClient *)clientTable[l].list; tmp;
			     i++, tmp = tmp->hnext)
			    {
				if (parv[1][2] == '1' && tmp != tmp->from)
					continue;
				sendto_one(sptr,"NOTICE %s :Node: %d #%d %s",
					   parv[0], l, i, tmp->name);
			    }
		return (0);
	    }
	case 'N' :
	    {
		aChannel *tmp;
		int	max;

		if (parc <= 2)
			return (0);
		l = atoi(parv[2]) % CHANNELHASHSIZE;
		if (parc > 3)
			max = atoi(parv[3]) % CHANNELHASHSIZE;
		else
			max = l;
		for (;l <= max; l++)
			for (i = 0, tmp = (aChannel *)channelTable[l].list; tmp;
			     i++, tmp = tmp->hnextch)
				sendto_one(sptr,"NOTICE %s :Node: %d #%d %s",
					   parv[0], l, i, tmp->chname);
		return (0);
	    }
	case 'S' :
#else
	if (parc>1&&!strcmp(parv[1],"sums")){
#endif
	sendto_one(sptr, "NOTICE %s :SUSER SSERV", parv[0]);
	sendto_one(sptr, "NOTICE %s :SBSDC IRCDC", parv[0]);
	sendto_one(sptr, "NOTICE %s :CHANC SMISC", parv[0]);
	sendto_one(sptr, "NOTICE %s :HASHC VERSH", parv[0]);
	sendto_one(sptr, "NOTICE %s :MAKEF HOSTID", parv[0]);
#ifndef	DEBUGMODE
	}
#endif
	    return 0;
#ifdef	DEBUGMODE
	case 'z' :
	    {
		Reg1	aClient	*acptr;

		if (parc <= 2)
			return 0;
		l = atoi(parv[2]);
		if (l < 256)
			return 0;
		(void)free((char *)clientTable);
		clientTable = (aHashEntry *)malloc(sizeof(aHashEntry) * l);
		HASHSIZE = l;
		clear_client_hash_table();
		for (acptr = client; acptr; acptr = acptr->next)
		    {
			acptr->hnext = NULL;
			(void)add_to_client_hash_table(acptr->name, acptr);
		    }
		sendto_one(sptr, "NOTICE %s :HASHSIZE now %d", parv[0], l);
		break;
	    }
	case 'Z' :
	    {
		Reg1	aChannel	*acptr;

		if (parc <= 2)
			return 0;
		l = atoi(parv[2]);
		if (l < 256)
			return 0;
		(void)free((char *)channelTable);
		channelTable = (aHashEntry *)malloc(sizeof(aHashEntry) * l);
		CHANNELHASHSIZE = l;
		clear_channel_hash_table();
		for (acptr = channel; acptr; acptr = acptr->nextch)
		    {
			acptr->hnextch = NULL;
			(void)add_to_channel_hash_table(acptr->chname, acptr);
		    }
		sendto_one(sptr, "NOTICE %s :CHANNELHASHSIZE now %d",
			   parv[0], l);
		break;
	    }
	default :
		break;
	}
	sendto_one(sptr,"NOTICE %s :Entries Hashed: %d NonEmpty: %d of %d",
		   parv[0], totlink, used_now, size);
	if (!used_now)
		used_now = 1;
	sendto_one(sptr,"NOTICE %s :Hash Ratio (av. depth): %f %Full: %f",
		  parv[0], (float)((1.0 * totlink) / (1.0 * used_now)),
		  (float)((1.0 * used_now) / (1.0 * size)));
	sendto_one(sptr,"NOTICE %s :Deepest Link: %d Links: %d",
		   parv[0], deeplink, deepest);
	if (!used)
		used = 1;
	sendto_one(sptr,"NOTICE %s :Total Hits: %d Unhit: %d Av Hits: %f",
		   parv[0], tothits, size-used,
		   (float)((1.0 * tothits) / (1.0 * used)));
	sendto_one(sptr,"NOTICE %s :Entry Most Hit: %d Hits: %d",
		   parv[0], mosthit, mosthits);
	sendto_one(sptr,"NOTICE %s :Client hits %d miss %d",
		   parv[0], clhits, clmiss);
	sendto_one(sptr,"NOTICE %s :Channel hits %d miss %d",
		   parv[0], chhits, chmiss);
	return 0;
#endif
}
