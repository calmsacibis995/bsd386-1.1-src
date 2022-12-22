/************************************************************************
 *   IRC - Internet Relay Chat, irc/c_msg.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *		      University of Oulu, Computing Center
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

/* -- Jto -- 03 Jun 1990
 * Fixes to allow channels to be strings
 */
 
#ifndef lint
char c_msg_id[] = "c_msg.c v2.0 (c) 1988 University of Oulu, Computing Center\
 and Jarkko Oikarinen";
#endif

#include "struct.h"
#ifdef AUTOMATON
#include <ctype.h>
#else
#ifndef VMSP
#include <curses.h>
#endif
#endif

#include "common.h"
#include "msg.h"
#if 0
#ifndef SOL20
#include "h.h"
#endif /* SOL20	Bad m_names and m_private prototypes	Vesa */
#endif /* 0	Nobody needs it. */

char	mybuf[513];

extern	anIgnore *find_ignore();
extern	char	*last_to_me(), userhost[];
extern	aClient	*client, me; /* 'me' from h.h	Vesa */

m_restart() {
  putline("*** Oh boy... somebody wants *me* to restart... exiting...\n");
  sleep(5);
  exit(0);
}

#ifdef MSG_MAIL
m_mail() {
  putline("*** Received base message..!");
}
#endif

m_time() {
  putline("*** Received time message..!");
}

m_motd() {
  putline("*** Received motd message..!");
}

m_admin() {
  putline("*** Received admin message..!");
}

m_service() {
  putline("*** Received service message..!");
}

m_trace() {
  putline("*** Received trace message..!");
}

m_rehash() {
  putline("*** Received rehash message..!");
}

m_die() {
  exit(-1);
}

m_pass() {
  putline("*** Received Pass message !");
}

m_oper() {
  putline("*** Received Oper message !");
}

m_names() {
  putline("*** Received Names message !");
}

m_lusers() {
  putline("*** Received Lusers message !");
}

m_mode(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  strcpy(mybuf, "*** Mode change ");
  while (parc--) {
    strcat(mybuf, parv[0]);
    strcat(mybuf, " ");
    parv++;
  }
  putline(mybuf);
}

m_wall(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf, "*** #%s# %s", parv[0], parv[1]);
  putline(mybuf);
}

m_wallops(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf, "*** =%s= %s", parv[0], parv[1]);
  putline(mybuf);
}

m_connect() {
  putline("*** Received Connect message !");
}

m_ping(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  if (parv[2] && *parv[2])
    sendto_one(client, "PONG %s@%s %s", client->user->username,
	       client->sockhost, parv[2]);
  else
    sendto_one(client, "PONG %s@%s", client->user->username, client->sockhost);
}

m_pong(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf, "*** Received PONG message: %s %s",
	  parv[1], (parv[2]) ? parv[2] : "");
  putline(mybuf);
}

int	m_nick(sptr, cptr, parc, parv)
aClient	*sptr, *cptr;
int	parc;
char	*parv[];
{
	sprintf(mybuf,"*** Change: %s is now known as %s", parv[0], parv[1]);
	if (!mycmp(me.name, parv[0])) {
		strcpy(me.name, parv[1]);
		write_statusline();
	}
	putline(mybuf);
#ifdef AUTOMATON
	a_nick(parv[0], parv[1]);
#endif
}

m_away(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** %s is away: \"%s\"",parv[0], parv[1]);
  putline(mybuf);
}

m_who() { 
  putline("*** Oh boy... server asking who from client... exiting...");
}

m_whois() {
  putline("*** Oh boy... server asking whois from client... exiting...");
}

m_whowas() {
  putline("*** Oh boy... server asking whowas from client... exiting...");
}

m_user() {
  putline("*** Oh boy... server telling me user messages... exiting...");
}

m_server(sptr, cptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** New server: %s", parv[1]);
  putline(mybuf);
}

m_list() {
  putline("*** Oh boy... server asking me channel lists... exiting...");
}

m_topic(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf, "*** %s changed the topic on %s to: %s",
	  parv[0], parv[1], parv[2]);
  putline(mybuf);
}

int	m_join(sptr, cptr, parc, parv)
aClient	*cptr, *sptr;
int	parc;
char	*parv[];
{
	sprintf(mybuf,"*** Change: %s has joined channel %s", 
		parv[0], parv[1]);
	putline(mybuf);
}

m_part(sptr, cptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Change: %s has left channel %s (%s)", 
	  parv[0], parv[1], BadPtr(parv[2]) ? parv[1] : parv[2]);
  putline(mybuf);
}

m_version(sptr, cptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Version: %s", parv[1]);
  putline(mybuf);
}

m_bye()
{
#if !defined(AUTOMATON) && !defined(VMSP)
  echo();
  nocrmode();
  clear();
  refresh();
#endif
  exit(-1);    
}

m_quit(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Signoff: %s (%s)", parv[0], parv[1]);
  putline(mybuf);
#ifdef AUTOMATON
  a_quit(sender);
#endif
}

m_kill(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Kill: %s (%s)", parv[0], parv[2]);
  putline(mybuf);
}

m_info(sptr, cptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Info: %s", parv[1]);
  putline(mybuf);
}

m_links() { 
  putline("*** Received LINKS message");
}

m_summon() {
  putline("*** Received SUMMON message");
}

m_stats() {
  putline("*** Received STATS message");
}

m_users() {
  putline("*** Received USERS message");
}

m_help() {
  putline("*** Received HELP message");
}

m_squit(sptr, cptr, parc, parv)
aClient *cptr, *sptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Server %s has died. Snif.", parv[1]);
  putline(mybuf);
}

m_whoreply(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  m_newwhoreply(parv[1], parv[2], parv[3], parv[5], parv[6], parv[7]);
  putline(mybuf);
}

m_newwhoreply(channel, username, host, nickname, away, realname)
char *channel, *username, *host, *nickname, *away, *realname;
{
  /* ...dirty hack, just assume all parameters present.. --msa */

  if (*away == 'S')
    sprintf(mybuf, "  %-13s    %s %-42s %s",
	"Nickname", "Chan", "Name", "<User@Host>");
  else
    {
    int i;
    char uh[USERLEN + HOSTLEN + 1];
    if (!realname)
      realname = "";
    if (!username)
      username = "";
    i = 50-strlen(realname)-strlen(username);

    if (channel[0] == '*')
	channel = "";

    if (strlen(host) > i)       /* kludge --argv */
	{
	host += strlen(host) - i;
	}

    sprintf(uh, "%s@%s", username, host);

    sprintf(mybuf, "%c %s%s %*s %s %*s",
	away[0], nickname, away+1,
	21-strlen(nickname)-strlen(away), channel,
	realname,
	53-strlen(realname), uh);
    }
}

#if 0
m_text(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  anIgnore *iptr;
  if ((iptr = find_ignore(parv[0], (anIgnore *)NULL, userhost)) &&
      (iptr->flags & IGNORE_PUBLIC))
      return(0);
  if (!BadPtr(parv[0])) {
    sprintf(mybuf,"<%s> %s", parv[0], parv[1]);
#ifdef AUTOMATON
    a_msg(mybuf);
#else
    putline(mybuf);
#endif
  } else
    putline(parv[1]);
}
#endif /* 0 Vesa */

m_namreply(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  parv[5] = parv[4];
  parv[4] = parv[3];
  parv[3] = parv[2];
  parv[2] = parv[1];
  m_newnamreply(sptr, cptr, parc, parv);
  putline(mybuf);
}

m_newnamreply(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  if (parv[2]) {
    switch (parv[2][0]) {
      case '*':
	sprintf(mybuf,"Prv: %-3s %s", parv[3], parv[4]);
	break;
      case '=':
	sprintf(mybuf,"Pub: %-3s %s", parv[3], parv[4]);
	break;
      case '@':
	sprintf(mybuf,"Sec: %-3s %s", parv[3], parv[4]);
	break;
      default:
	sprintf(mybuf,"???: %s %s %s", parv[3], parv[4], parv[5]);
	break;
    }
  } else
    sprintf(mybuf, "*** Internal Error: namreply");
}

m_linreply(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Server: %s (%s) %s", parv[2], parv[3], parv[4]);
}

int	m_private(sptr, cptr, parc, parv)
aClient	*sptr, *cptr;
int	parc;
char	*parv[];
{
	anIgnore *iptr;

	iptr = find_ignore(parv[0], (anIgnore *)NULL, userhost);
	if ((iptr != (anIgnore *)NULL) && iptr->flags & IGNORE_PRIVATE)
		return 0;
	check_ctcp(sptr, cptr, parc, parv);

	if (parv[0] && *parv[0]) {
#ifdef AUTOMATON
		a_privmsg(sender, parv[2]);
#else
		if (((*parv[1] >= '0') && (*parv[1] <= '9')) ||
		    (*parv[1] == '-') || (*parv[1] == '+') ||
		    IsChannelName(parv[1]) || (*parv[1] == '$')) {
			sprintf(mybuf,"<%s:%s> %s", parv[0], parv[1], parv[2]);
		} else {
			sprintf(mybuf,"*%s* %s", parv[0], parv[2]);
			last_to_me(parv[0]);
		}
		putline(mybuf);
#endif
	} else
		putline(parv[2]); /* parv[2] and no parv[0] ?! - avalon */
	return 0;
}


m_kick(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
	sprintf(mybuf,"*** %s kicked %s off channel %s (%s)",
		(parv[0]) ? parv[0] : "*Unknown*",
		(parv[2]) ? parv[2] : "*Unknown*",
		(parv[1]) ? parv[1] : "*Unknown*",
		parv[3]);
	putline(mybuf);
}

int	m_notice(sptr, cptr, parc, parv)
aClient	*sptr, *cptr;
int	parc;
char	*parv[];
{
	if (parv[0] && parv[0][0]) {
		if ((*parv[1] >= '0' && *parv[1] <= '9') ||
		    *parv[1] == '-' || IsChannelName(parv[1]) ||
		    *parv[1] == '$' || *parv[1] == '+')
			sprintf(mybuf,"(%s:%s) %s",parv[0],parv[1],parv[2]);
		else
			sprintf(mybuf, "-%s- %s", parv[0], parv[2]);
		putline(mybuf);
	} else
		putline(parv[2]);
}

m_invite(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  anIgnore *iptr;
  if ((iptr = find_ignore(parv[0], (anIgnore *)NULL, userhost)) &&
      (iptr->flags & IGNORE_PRIVATE) && (iptr->flags & IGNORE_PUBLIC)) {
	sendto_one(client,
		   "NOTICE %s :*** Automatic reply: You have been ignored",
		   parv[0]);
	return(0);
      }
#ifdef AUTOMATON
  a_invite(parv[0], parv[2]);
#else
  sprintf(mybuf,"*** %s Invites you to channel %s", parv[0], parv[2]);
  putline(mybuf);
#endif
  return(0);
}

m_error(sptr, cptr, parc, parv)
aClient *sptr, *cptr;
int parc;
char *parv[];
{
  sprintf(mybuf,"*** Error: %s %s", parv[1], (parv[2]) ? parv[2] : "");
  putline(mybuf);
}

m_userhost() {
  putline("*** Oh boy... server asking userhost from client... exiting...");
}

m_ison() {
  putline("*** Oh boy... server asking ison from client... exiting...");
}

m_close() {
  putline("*** Oh boy... server asking close from client... exiting...");
}
