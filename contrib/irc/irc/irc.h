/************************************************************************
 *   IRC - Internet Relay Chat, irc/irc.h
 *   Copyright (C) 1990 Jarkko Oikarinen
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

/* -- Jto -- 07 Jul 1990
 * Added mail command
 */

struct Command {
  int (*func)();
  char *name;
  int type;
  char keybinding[3];
  char *extra;   /* Normally contains the command to send to irc daemon */
};

#define SERVER_CMD   0
#define LOCAL_FUNC  1

extern int do_mypriv(), do_cmdch(), do_quote(), do_query();
extern int do_ignore(), do_help(), do_log(), do_clear();
extern int do_unkill(), do_bye(), do_kill(), do_kick();
extern int do_server(), do_channel(), do_away();
#ifdef VMSP
extern int do_bye(), do_exec();
#endif
#ifdef GETPASS
extern int do_oper();
#endif

struct Command commands[] = {
  { (int (*)()) 0, "SIGNOFF", SERVER_CMD, "\0\0", MSG_QUIT },
  { do_bye,        "QUIT",    LOCAL_FUNC, "\0\0", MSG_QUIT },
  { do_bye,        "EXIT",    LOCAL_FUNC, "\0\0", MSG_QUIT },
  { do_bye,        "BYE",     LOCAL_FUNC, "\0\0", MSG_QUIT },
  { do_kill,       "KILL",    LOCAL_FUNC, "\0\0", MSG_KILL },
  { (int (*)()) 0, "SUMMON",  SERVER_CMD, "\0\0", MSG_SUMMON },
  { (int (*)()) 0, "STATS",   SERVER_CMD, "\0\0", MSG_STATS },
  { (int (*)()) 0, "USERS",   SERVER_CMD, "\0\0", MSG_USERS },
  { (int (*)()) 0, "TIME",    SERVER_CMD, "\0\0", MSG_TIME },
  { (int (*)()) 0, "DATE",    SERVER_CMD, "\0\0", MSG_TIME },
  { (int (*)()) 0, "NAMES",   SERVER_CMD, "\0\0", MSG_NAMES },
  { (int (*)()) 0, "NICK",    SERVER_CMD, "\0\0", MSG_NICK },
  { (int (*)()) 0, "WHO",     SERVER_CMD, "\0\0", MSG_WHO },
  { (int (*)()) 0, "WHOIS",   SERVER_CMD, "\0\0", MSG_WHOIS },
  { (int (*)()) 0, "WHOWAS",  SERVER_CMD, "\0\0", MSG_WHOWAS },
  { do_kill,	   "LEAVE",   LOCAL_FUNC, "\0\0", MSG_PART },
  { do_kill,	   "PART",    LOCAL_FUNC, "\0\0", MSG_PART },
  { (int (*)()) 0, "WOPS",    SERVER_CMD, "\0\0", MSG_WALLOPS },
  { do_channel,    "JOIN",    LOCAL_FUNC, "\0\0", MSG_JOIN },
  { do_channel,    "CHANNEL", LOCAL_FUNC, "\0\0", MSG_JOIN },
#ifdef VMSP
  { do_exec,       "EXEC",    LOCAL_FUNC, "\0\0", "EXEC" },
  { do_oper,       "OPER",    LOCAL_FUNC, "\0\0", "OPER" },
#endif
#ifdef GETPASS
  { do_oper,       "OPER",    LOCAL_FUNC, "\0\0", "OPER" },
#else
  { (int (*)()) 0, "OPER",    SERVER_CMD, "\0\0", MSG_OPER },
#endif
  { do_away,	   "AWAY",    LOCAL_FUNC, "\0\0", MSG_AWAY },
  { do_mypriv,     "MSG",     LOCAL_FUNC, "\0\0", MSG_PRIVATE },
  { do_kill,       "TOPIC",   LOCAL_FUNC, "\0\0", MSG_TOPIC },
  { do_cmdch,      "CMDCH",   LOCAL_FUNC, "\0\0", "CMDCH" },
  { (int (*)()) 0, "INVITE",  SERVER_CMD, "\0\0", MSG_INVITE },
  { (int (*)()) 0, "INFO",    SERVER_CMD, "\0\0", MSG_INFO },
  { (int (*)()) 0, "LIST",    SERVER_CMD, "\0\0", MSG_LIST },
  { (int (*)()) 0, "KILL",    SERVER_CMD, "\0\0", MSG_KILL },
  { do_quote,      "QUOTE",   LOCAL_FUNC, "\0\0", "QUOTE" },
  { (int (*)()) 0, "LINKS",   SERVER_CMD, "\0\0", MSG_LINKS },
  { (int (*)()) 0, "ADMIN",   SERVER_CMD, "\0\0", MSG_ADMIN },
  { do_ignore,     "IGNORE",  LOCAL_FUNC, "\0\0", "IGNORE" },
  { (int (*)()) 0, "TRACE",   SERVER_CMD, "\0\0", MSG_TRACE },
  { do_help,       "HELP",    LOCAL_FUNC, "\0\0", "HELP" },
  { do_log,        "LOG",     LOCAL_FUNC, "\0\0", "LOG" },
  { (int (*)()) 0, "VERSION", SERVER_CMD, "\0\0", MSG_VERSION },
  { do_clear,      "CLEAR",   LOCAL_FUNC, "\0\0", "CLEAR" },
  { (int (*)()) 0, "REHASH",  SERVER_CMD, "\0\0", MSG_REHASH },
  { do_query,      "QUERY",   LOCAL_FUNC, "\0\0", "QUERY" },
  { (int (*)()) 0, "LUSERS",  SERVER_CMD, "\0\0", MSG_LUSERS },
  { (int (*)()) 0, "MOTD",    SERVER_CMD, "\0\0", MSG_MOTD },
  { do_unkill,     "UNKILL",  LOCAL_FUNC, "\0\0", "UNKILL" },
  { do_server,     "SERVER",  LOCAL_FUNC, "\0\0", "SERVER" },
  { (int (*)()) 0, "MODE",    SERVER_CMD, "\0\0", MSG_MODE },
#ifdef MSG_MAIL
  { (int (*)()) 0, "MAIL",    SERVER_CMD, "\0\0", MSG_MAIL },
#endif
  { do_kick,       "KICK",    LOCAL_FUNC, "\0\0", MSG_KICK },
  { (int (*)()) 0, "USERHOST",SERVER_CMD, "\0\0", MSG_USERHOST },
  { (int (*)()) 0, "ISON",    SERVER_CMD, "\0\0", MSG_ISON },
  { (int (*)()) 0, "CONNECT", SERVER_CMD, "\0\0", MSG_CONNECT },
  { do_kill,       "SQUIT",   LOCAL_FUNC, "\0\0", MSG_SQUIT },
  { (int (*)()) 0, (char *) 0, 0,         "\0\0", (char *) 0 }
};
