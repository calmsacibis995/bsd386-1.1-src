/************************************************************************
 *   IRC - Internet Relay Chat, include/h.h
 *   Copyright (C) 1992 Darren Reed
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
 * "h.h". - Headers file.
 *
 * Most of the externs and prototypes thrown in here to 'cleanup' things.
 * -avalon
 */

extern	time_t	nextconnect, nextdnscheck, nextping;
extern	aClient	*client, me, *local[];
extern	aChannel *channel;
extern	struct	stats	*ircstp;
extern	int	bootopt;

extern	aChannel *find_channel PROTO((char *, aChannel *));
extern	void	remove_user_from_channel PROTO((aClient *, aChannel *));
extern	void	del_invite PROTO((aClient *, aChannel *));
extern	void	send_user_joins PROTO((aClient *, aClient *));
extern	void	clean_channelname PROTO((char *));
extern	int	can_send PROTO((aClient *, aChannel *));
extern	int	is_chan_op PROTO((aClient *, aChannel *));
extern	int	has_voice PROTO((aClient *, aChannel *));
extern	int	count_channels PROTO((aClient *));

extern	aClient	*find_client PROTO((char *, aClient *));
extern	aClient	*find_name PROTO((char *, aClient *));
extern	aClient	*find_person PROTO((char *, aClient *));
extern	aClient	*find_server PROTO((char *, aClient *));
extern	aClient	*find_service PROTO((char *, aClient *));
extern	aClient	*find_userhost PROTO((char *, char *, aClient *, int *));

extern	int	attach_conf PROTO((aClient *, aConfItem *));
extern	aConfItem *attach_confs PROTO((aClient*, char *, int));
extern	aConfItem *attach_confs_host PROTO((aClient*, char *, int));
extern	int	attach_Iline PROTO((aClient *, struct hostent *, char *));
extern	aConfItem *conf, *find_me PROTO(()), *find_admin PROTO(());
extern	aConfItem *count_cnlines PROTO((Link *));
extern	void	det_confs_butmask PROTO((aClient *, int));
extern	int	detach_conf PROTO((aClient *, aConfItem *));
extern	aConfItem *det_confs_butone PROTO((aClient *, aConfItem *));
extern	aConfItem *find_conf PROTO((Link *, char*, int));
extern	aConfItem *find_conf_exact PROTO((char *, char *, char *, int));
extern	aConfItem *find_conf_host PROTO((Link *, char *, int));
extern	aConfItem *find_conf_ip PROTO((Link *, char *, char *, int));
extern	aConfItem *find_conf_name PROTO((char *, int));
extern	int	find_kill PROTO((aClient *));
extern	int	find_restrict PROTO((aClient *));
extern	int	rehash PROTO((aClient *, aClient *, int));
extern	char	*MyMalloc PROTO((int)), *MyRealloc PROTO((char *, int));
extern	char	*debugmode, *configfile;
extern	char	*getfield PROTO((char *));
extern	void	get_sockhost PROTO((aClient *, char *));
extern	char	*rpl_str PROTO((int)), *err_str PROTO((int));
extern	char	*strerror PROTO((int));
extern	int	dgets PROTO((int, char *, int));
extern	char	*inetntoa PROTO((char *));

extern	int	dbufalloc, dbufblocks, debuglevel, errno, h_errno;
extern	int	highest_fd, debuglevel, portnum, debugtty, maxusersperchannel;
extern	int	readcalls, udpfd, resfd;
extern	aClient	*add_connection PROTO((aClient *, int));
extern	int	add_listener PROTO((aConfItem *));
extern	void	add_local_domain PROTO((char *, int));
extern	int	check_client PROTO((aClient *));
extern	int	check_server PROTO((aClient *, struct hostent *, \
				    aConfItem *, aConfItem *, int));
extern	int	check_server_init PROTO((aClient *));
extern	void	close_connection PROTO((aClient *));
extern	void	close_listeners PROTO(());
extern	int connect_server PROTO((aConfItem *, aClient *, struct hostent *));
extern	void	get_my_name PROTO((aClient *, char *, int));
extern	int	get_sockerr PROTO((aClient *));
extern	int	inetport PROTO((aClient *, char *, int));
extern	void	init_sys PROTO(());
extern	int	read_message PROTO((time_t));
extern	void	report_error PROTO((char *, aClient *));
extern	void	set_non_blocking PROTO((int, aClient *));
extern	int	setup_ping PROTO(());
extern	void	summon PROTO((aClient *, char *, char *, char *));
extern	int	unixport PROTO((aClient *, char *, int));
extern	int	utmp_open PROTO(());
extern	int	utmp_read PROTO((int, char *, char *, char *, int));
extern	int	utmp_close PROTO((int));

extern	void	start_auth PROTO((aClient *));
extern	void	read_authports PROTO((aClient *));
extern	void	send_authports PROTO((aClient *));

extern	void	restart PROTO((char *));
extern	void	send_channel_modes PROTO((aClient *, aChannel *));
extern	void	server_reboot PROTO(());
extern	void	terminate PROTO(()), write_pidfile PROTO(());

extern	int	send_queued PROTO((aClient *));
/*VARARGS2*/
extern	void	sendto_one();
/*VARARGS4*/
extern	void	sendto_channel_butone();
/*VARARGS2*/
extern	void	sendto_serv_butone();
/*VARARGS2*/
extern	void	sendto_common_channels();
/*VARARGS3*/
extern	void	sendto_channel_butserv();
/*VARARGS3*/
extern	void	sendto_match_servs();
/*VARARGS5*/
extern	void	sendto_match_butone();
/*VARARGS3*/
extern	void	sendto_all_butone();
/*VARARGS1*/
extern	void	sendto_ops();
/*VARARGS3*/
extern	void	sendto_ops_butone();
/*VARARGS3*/
extern	void	sendto_prefix_one();

extern	int	writecalls, writeb[];
extern	int	deliver_it PROTO((aClient *, char *, int));

extern	int	check_registered PROTO((aClient *));
extern	int	check_registered_user PROTO((aClient *));
extern	char	*get_client_name PROTO((aClient *, int));
extern	char	*my_name_for_link PROTO((char *, aConfItem *));
extern	char	*myctime PROTO((time_t)), *date PROTO((time_t));
extern	int	exit_client PROTO((aClient *, aClient *, aClient *, char *));
extern	void	initstats PROTO(()), tstats PROTO((aClient *, char *));

extern	int	parse PROTO((aClient *, char *, char *, struct Message *));
extern	int	do_numeric PROTO((int, aClient *, aClient *, int, char **));
extern	int hunt_server PROTO((aClient *,aClient *,char *,int,int,char **));
extern	aClient	*next_client PROTO((aClient *, char *));
#ifndef	CLIENT_COMPILE
extern	int	m_umode PROTO((aClient *, aClient *, int, char **));
extern	int	m_names PROTO((aClient *, aClient *, int, char **));
extern	int	m_server_estab PROTO((aClient *));
extern	void	send_umode PROTO((aClient *, aClient *, int, int, char *));
extern	void	send_umode_out PROTO((aClient*, aClient *, int));
#endif

extern	void	free_client PROTO((aClient *));
extern	void	free_link PROTO((Link *));
extern	void	free_conf PROTO((aConfItem *));
extern	void	free_class PROTO((aClass *));
extern	void	free_user PROTO((anUser *, aClient *));
extern	Link	*make_link PROTO(());
extern	anUser	*make_user PROTO((aClient *));
extern	aConfItem *make_conf PROTO(());
extern	aClass	*make_class PROTO(());
extern	aServer	*make_server PROTO(());
extern	aClient	*make_client PROTO((aClient *));
extern	Link	*find_user_link PROTO((Link *, aClient *));
extern	void	add_client_to_list PROTO((aClient *));
extern	void	checklist PROTO(());
extern	void	remove_client_from_list PROTO((aClient *));
extern	void	initlists PROTO(());

extern	void	add_class PROTO((int, int, int, int, long));
extern	long	get_sendq PROTO((aClient *));
extern	int	get_con_freq PROTO((aClass *));
extern	int	get_client_ping PROTO((aClient *));
extern	int	get_client_class PROTO((aClient *));
extern	int	get_conf_class PROTO((aConfItem *));
extern	void	report_classes PROTO((aClient *));

extern	struct	hostent	*get_res PROTO((char *));
extern	struct	hostent	*gethost_byaddr PROTO((char *, Link *));
extern	struct	hostent	*gethost_byname PROTO((char *, Link *));
extern	void	flush_cache PROTO(());
extern	int	init_resolver PROTO((int));
extern	time_t	timeout_query_list PROTO((time_t));
extern	time_t	expire_cache PROTO((time_t));
extern	void    del_queries PROTO((char *));

extern	void	clear_channel_hash_table PROTO(());
extern	void	clear_client_hash_table PROTO(());
extern	int	add_to_client_hash_table PROTO((char *, aClient *));
extern	int	del_from_client_hash_table PROTO((char *, aClient *));
extern	int	add_to_channel_hash_table PROTO((char *, aChannel *));
extern	int	del_from_channel_hash_table PROTO((char *, aChannel *));
extern	aChannel *hash_find_channel PROTO((char *, aChannel *));
extern	aClient	*hash_find_client PROTO((char *, aClient *));
extern	aClient	*hash_find_nickserver PROTO((char *, aClient *));
extern	aClient	*hash_find_server PROTO((char *, aClient *));

extern	void	add_history PROTO((aClient *));
extern	aClient	*get_history PROTO((char *, time_t));
extern	void	initwhowas PROTO(());
extern	void	off_history PROTO((aClient *));

extern	int	dopacket PROTO((aClient *, char *, int));

#ifdef	CLIENT_COMPILE
extern	char	*mycncmp PROTO((char *, char *));
#endif

/*VARARGS2*/
extern	void	debug();
#if defined(DEBUGMODE) && !defined(CLIENT_COMPILE)
extern	void	send_usage PROTO((aClient *, char *));
extern	void	send_listinfo PROTO((aClient *, char *));
extern	void	count_memory PROTO((aClient *, char *));
#endif
