/*
 * The mrouted program is covered by the license in the accompanying file
 * named "LICENSE".  Use of the mrouted program represents acceptance of
 * the terms and conditions listed in that file.
 *
 * The mrouted program is COPYRIGHT 1989 by The Board of Trustees of
 * Leland Stanford Junior University.
 *
 *
 * Stanford Id: vif.c,v 1.5 1993/06/24 05:11:16 deering Exp
 */


#include "defs.h"


/*
 * Exported variables.
 */
struct uvif	uvifs[MAXVIFS];	/* array of virtual interfaces */
vifi_t		numvifs;	/* number of vifs in use       */
int		vifs_down;	/* 1=>some interfaces are down */
int		udp_socket;	/* Since the honkin' kernel doesn't support */
				/* ioctls on raw IP sockets, we need a UDP  */
				/* socket as well as our IGMP (raw) socket. */
				/* How dumb.                                */

/*
 * Forward declarations.
 */
static void start_vif();
static void stop_vif();


/*
 * Initialize the virtual interfaces.
 */
void init_vifs()
{
    vifi_t vifi;
    struct uvif *v;
    int enabled_vifs, enabled_phyints;

    numvifs = 0;
    vifs_down = FALSE;

    /*
     * Configure the vifs based on the interface configuration of the
     * the kernel and the contents of the configuration file.
     * (Open a UDP socket for ioctl use in the config procedures.)
     */
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	log(LOG_ERR, errno, "UDP socket");
    config_vifs_from_kernel();
    config_vifs_from_file();

    /*
     * Quit if there are fewer than two enabled vifs.
     */
    enabled_vifs    = 0;
    enabled_phyints = 0;
    for (vifi = 0, v = uvifs; vifi < numvifs; ++vifi, ++v) {
	if (!(v->uv_flags & VIFF_DISABLED)) {
	    ++enabled_vifs;
	    if (!(v->uv_flags & VIFF_TUNNEL))
		++enabled_phyints;
	}
    }
    if (enabled_vifs < 2)
	log(LOG_ERR, 0, "can't forward: %s",
	    enabled_vifs == 0 ? "no enabled vifs" : "only one enabled vif");

    if (enabled_phyints == 0)
	log(LOG_WARNING, 0,
	    "no enabled interfaces, forwarding via tunnels only");

    /*
     * Start routing on all virtual interfaces that are not down or
     * administratively disabled.
     */
    for (vifi = 0, v = uvifs; vifi < numvifs; ++vifi, ++v) {
	if (!(v->uv_flags & VIFF_DISABLED)) {
	    if (!(v->uv_flags & VIFF_DOWN))
		start_vif(vifi);
	    else log(LOG_INFO, 0,
		    "%s is not yet up; vif #%u not in service",
		    v->uv_name, vifi);
	}
    }
}


/*
 * See if any interfaces have changed from up state to down, or vice versa,
 * including any non-multicast-capable interfaces that are in use as local
 * tunnel end-points.  Ignore interfaces that have been administratively
 * disabled.
 */
void check_vif_state()
{
    register vifi_t vifi;
    register struct uvif *v;
    struct ifreq ifr;

    vifs_down = FALSE;
    for (vifi = 0, v = uvifs; vifi < numvifs; ++vifi, ++v) {

	if (v->uv_flags & VIFF_DISABLED) continue;

	strncpy(ifr.ifr_name, v->uv_name, IFNAMSIZ);
	if (ioctl(udp_socket, SIOCGIFFLAGS, (char *)&ifr) < 0)
	    log(LOG_ERR, errno,
		"ioctl SIOCGIFFLAGS for %s", ifr.ifr_name);

	if (v->uv_flags & VIFF_DOWN) {
	    if (ifr.ifr_flags & IFF_UP) {
		v->uv_flags &= ~VIFF_DOWN;
		start_vif(vifi);
		log(LOG_INFO, 0,
		    "%s has come up; vif #%u now in service",
		    v->uv_name, vifi);
	    }
	    else vifs_down = TRUE;
	}
	else {
	    if (!(ifr.ifr_flags & IFF_UP)) {
		stop_vif(vifi);
		v->uv_flags |= VIFF_DOWN;
		log(LOG_INFO, 0,
		    "%s has gone down; vif #%u taken out of service",
		    v->uv_name, vifi);
		vifs_down = TRUE;
	    }
	}
    }
}


/*
 * Start routing on the specified virtual interface.
 */
static void start_vif(vifi)
    vifi_t vifi;
{
    struct uvif *v;
    u_long src, dst;

    v   = &uvifs[vifi];
    src = v->uv_lcl_addr;
    dst = (v->uv_flags & VIFF_TUNNEL) ? v->uv_rmt_addr : dvmrp_group;

    /*
     * Install the interface in the kernel's vif structure.
     */
    k_add_vif(vifi, &uvifs[vifi]);

    /*
     * Update the existing route entries to take into account the new vif.
     */
    add_vif_to_routes(vifi);

    if (!(v->uv_flags & VIFF_TUNNEL)) {
	/*
	 * Join the DVMRP multicast group on the interface.
	 * (This is not strictly necessary, since the kernel promiscuously
	 * receives IGMP packets addressed to ANY IP multicast group while
	 * multicast routing is enabled.  However, joining the group allows
	 * this host to receive non-IGMP packets as well, such as 'pings'.)
	 */
	k_join(dvmrp_group, src);

	/*
	 * Install an entry in the routing table for the subnet to which
	 * the interface is connected.
	 */
	start_route_updates();
	update_route(v->uv_subnet, v->uv_subnetmask, 0, 0, vifi);

	/*
	 * Until neighbors are discovered, assume responsibility for sending
	 * periodic group membership queries to the subnet.  Send the first
	 * query.
	 */
	v->uv_flags |= VIFF_QUERIER;
	send_igmp(src, allhosts_group, IGMP_HOST_MEMBERSHIP_QUERY, 0, 0, 0);
    }

    /*
     * Send a probe via the new vif to look for neighbors.
     */
    send_igmp(src, dst, IGMP_DVMRP, DVMRP_PROBE, htonl(MROUTED_LEVEL), 0);
}


/*
 * Stop routing on the specified virtual interface.
 */
static void stop_vif(vifi)
    vifi_t vifi;
{
    struct uvif *v;
    struct listaddr *a;

    v = &uvifs[vifi];

    if (!(v->uv_flags & VIFF_TUNNEL)) {
	/*
	 * Depart from the DVMRP multicast group on the interface.
	 */
	k_leave(dvmrp_group, v->uv_lcl_addr);

	/*
	 * Update the entry in the routing table for the subnet to which
	 * the interface is connected, to take into account the interface
	 * failure.
	 */
	start_route_updates();
	update_route(v->uv_subnet, v->uv_subnetmask, UNREACHABLE, 0, vifi);

	/*
	 * Discard all group addresses.  (No need to tell kernel;
	 * the k_del_vif() call, below, will clean up kernel state.)
	 */
	while (v->uv_groups != NULL) {
	    a = v->uv_groups;
	    v->uv_groups = a->al_next;
	    free((char *)a);
	}

	v->uv_flags &= ~VIFF_QUERIER;
    }

    /*
     * Update the existing route entries to take into account the vif failure.
     */
    delete_vif_from_routes(vifi);

    /*
     * Delete the interface from the kernel's vif structure.
     */
    k_del_vif(vifi);

    /*
     * Discard all neighbor addresses.
     */
    while (v->uv_neighbors != NULL) {
	a = v->uv_neighbors;
	v->uv_neighbors = a->al_next;
	free((char *)a);
    }
}


/*
 * Find the virtual interface from which an incoming packet arrived,
 * based on the packet's source and destination IP addresses.
 */
vifi_t find_vif(src, dst)
    register u_long src;
    register u_long dst;
{
    register vifi_t vifi;
    register struct uvif *v;

    for (vifi = 0, v = uvifs; vifi < numvifs; ++vifi, ++v) {
	if (!(v->uv_flags & (VIFF_DOWN|VIFF_DISABLED))) {
	    if (v->uv_flags & VIFF_TUNNEL) {
		if (src == v->uv_rmt_addr && dst == v->uv_lcl_addr)
		    return(vifi);
	    }
	    else {
		if ((src & v->uv_subnetmask) == v->uv_subnet &&
		    src != v->uv_subnetbcast)
		    return(vifi);
	    }
	}
    }
    return (NO_VIF);
}


/*
 * Send group membership queries to all subnets for which I am querier.
 */
void query_groups()
{
    register vifi_t vifi;
    register struct uvif *v;

    for (vifi = 0, v = uvifs; vifi < numvifs; vifi++, v++) {
	if (v->uv_flags & VIFF_QUERIER) {
	    send_igmp(v->uv_lcl_addr, allhosts_group,
		      IGMP_HOST_MEMBERSHIP_QUERY, 0, 0, 0);
	}
    }
}


/*
 * Process an incoming group membership report.
 */
void accept_group_report(src, dst, group)
    u_long src, dst, group;
{
    register vifi_t vifi;
    register struct uvif *v;
    register struct listaddr *g;

    if ((vifi = find_vif(src, dst)) == NO_VIF ||
	(uvifs[vifi].uv_flags & VIFF_TUNNEL)) {
	log(LOG_INFO, 0,
	    "ignoring group membership report from non-adjacent host %s",
	    inet_fmt(src, s1));
	return;
    }

    v = &uvifs[vifi];

    /*
     * Look for the group in our group list; if found, reset its timer.
     */
    for (g = v->uv_groups; g != NULL; g = g->al_next) {
	if (group == g->al_addr) {
	    g->al_timer = 0;
	    break;
	}
    }

    /*
     * If not found, add it to the list and tell the kernel.
     */
    if (g == NULL) {
	g = (struct listaddr *)malloc(sizeof(struct listaddr));
	if (g == NULL)
	    log(LOG_ERR, 0, "ran out of memory");    /* fatal */

	g->al_addr   = group;
	g->al_timer  = 0;
	g->al_next   = v->uv_groups;
	v->uv_groups = g;

	k_add_group(vifi, group);
    }
}


/*
 * Send a probe on all vifs from which no neighbors have been heard recently.
 */
void probe_for_neighbors()
{
    register vifi_t vifi;
    register struct uvif *v;

    for (vifi = 0, v = uvifs; vifi < numvifs; vifi++, v++) {
	if (!(v->uv_flags & (VIFF_DOWN|VIFF_DISABLED)) &&
	    v->uv_neighbors == NULL) {
	    send_igmp(v->uv_lcl_addr,
		      (v->uv_flags & VIFF_TUNNEL) ? v->uv_rmt_addr
						  : dvmrp_group,
		      IGMP_DVMRP, DVMRP_PROBE, htonl(MROUTED_LEVEL), 0);
	}
    }
}


/*
 * Send a list of all of our neighbors to the requestor, `src'.
 */
void accept_neighbor_request(src, dst)
    u_long src, dst;
{
    vifi_t vifi;
    struct uvif *v;
    u_char *p, *ncount;
    struct listaddr *la;
    int	datalen;
    u_long temp_addr, us, them = src;

    /* Determine which of our addresses to use as the source of our response
     * to this query.
     */
    if (IN_MULTICAST(ntohl(dst))) { /* query sent to a multicast group */
	int udp;		/* find best interface to reply on */
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = dst;
	addr.sin_port = htons(2000); /* any port over 1024 will do... */
	if ((udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0
	    || connect(udp, (struct sockaddr *) &addr, sizeof(addr)) < 0
	    || getsockname(udp, (struct sockaddr *) &addr, &addrlen) < 0) {
	    log(LOG_WARNING, errno, "Determining local address");
	    close(udp);
	    return;
	}
	close(udp);
	us = addr.sin_addr.s_addr;
    } else			/* query sent to us alone */
	us = dst;	

#define PUT_ADDR(a)	temp_addr = ntohl(a); \
			*p++ = temp_addr >> 24; \
			*p++ = (temp_addr >> 16) & 0xFF; \
			*p++ = (temp_addr >> 8) & 0xFF; \
			*p++ = temp_addr & 0xFF;

    p = (u_char *) (send_buf + MIN_IP_HEADER_LEN + IGMP_MINLEN);
    datalen = 0;

    for (vifi = 0, v = uvifs; vifi < numvifs; vifi++, v++) {
	if (v->uv_flags & VIFF_DISABLED)
	    continue;

	ncount = 0;

	for (la = v->uv_neighbors; la; la = la->al_next) {

	    /* Make sure that there's room for this neighbor... */
	    if (datalen + (ncount == 0 ? 4 + 3 + 4 : 4) > MAX_DVMRP_DATA_LEN) {
		send_igmp(us, them, IGMP_DVMRP, DVMRP_NEIGHBORS,
			  htonl(MROUTED_LEVEL), datalen);
		p = (u_char *) (send_buf + MIN_IP_HEADER_LEN + IGMP_MINLEN);
		datalen = 0;
		ncount = 0;
	    }

	    /* Put out the header for this neighbor list... */
	    if (ncount == 0) {
		PUT_ADDR(v->uv_lcl_addr);
		*p++ = v->uv_metric;
		*p++ = v->uv_threshold;
		ncount = p;
		*p++ = 0;
		datalen += 4 + 3;
	    }

	    PUT_ADDR(la->al_addr);
	    datalen += 4;
	    (*ncount)++;
	}
    }

    if (datalen != 0)
	send_igmp(us, them, IGMP_DVMRP, DVMRP_NEIGHBORS, htonl(MROUTED_LEVEL),
		  datalen);
}

/*
 * Send a list of all of our neighbors to the requestor, `src'.
 */
void accept_neighbor_request2(src, dst)
    u_long src, dst;
{
    vifi_t vifi;
    struct uvif *v;
    u_char *p, *ncount;
    struct listaddr *la;
    int	datalen;
    u_long temp_addr, us, them = src;

    /* Determine which of our addresses to use as the source of our response
     * to this query.
     */
    if (IN_MULTICAST(ntohl(dst))) { /* query sent to a multicast group */
	int udp;		/* find best interface to reply on */
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = dst;
	addr.sin_port = htons(2000); /* any port over 1024 will do... */
	if ((udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0
	    || connect(udp, (struct sockaddr *) &addr, sizeof(addr)) < 0
	    || getsockname(udp, (struct sockaddr *) &addr, &addrlen) < 0) {
	    log(LOG_WARNING, errno, "Determining local address");
	    close(udp);
	    return;
	}
	close(udp);
	us = addr.sin_addr.s_addr;
    } else			/* query sent to us alone */
	us = dst;	

    p = (u_char *) (send_buf + MIN_IP_HEADER_LEN + IGMP_MINLEN);
    datalen = 0;

    for (vifi = 0, v = uvifs; vifi < numvifs; vifi++, v++) {
	register u_short vflags = v->uv_flags;
	register u_char rflags = 0;
	if (vflags & VIFF_TUNNEL)
	    rflags |= DVMRP_NF_TUNNEL;
	if (vflags & VIFF_SRCRT)
	    rflags |= DVMRP_NF_SRCRT;
	if (vflags & VIFF_DOWN)
	    rflags |= DVMRP_NF_DOWN;
	if (vflags & VIFF_DISABLED)
	    rflags |= DVMRP_NF_DISABLED;
	if (vflags & VIFF_QUERIER)
	    rflags |= DVMRP_NF_QUERIER;
	ncount = 0;
	la = v->uv_neighbors;
	if (la == NULL) {
		/*
		 * include down & disabled interfaces and interfaces on
		 * leaf nets.
		 */
		if (rflags & DVMRP_NF_TUNNEL)
		    rflags |= DVMRP_NF_DOWN;
		if (datalen > MAX_DVMRP_DATA_LEN - 12) {
		    send_igmp(us, them, IGMP_DVMRP, DVMRP_NEIGHBORS2,
			      htonl(MROUTED_LEVEL), datalen);
		    p = (u_char *) (send_buf + MIN_IP_HEADER_LEN + IGMP_MINLEN);
		    datalen = 0;
		}
		*(u_int*)p = v->uv_lcl_addr;
		p += 4;
		*p++ = v->uv_metric;
		*p++ = v->uv_threshold;
		*p++ = rflags;
		*p++ = 1;
		*(u_int*)p =  v->uv_rmt_addr;
		p += 4;
		datalen += 12;
	} else {
	    for ( ; la; la = la->al_next) {
		/* Make sure that there's room for this neighbor... */
		if (datalen + (ncount == 0 ? 4+4+4 : 4) > MAX_DVMRP_DATA_LEN) {
		    send_igmp(us, them, IGMP_DVMRP, DVMRP_NEIGHBORS2,
			      htonl(MROUTED_LEVEL), datalen);
		    p = (u_char *) (send_buf + MIN_IP_HEADER_LEN + IGMP_MINLEN);
		    datalen = 0;
		    ncount = 0;
		}
		/* Put out the header for this neighbor list... */
		if (ncount == 0) {
		    *(u_int*)p = v->uv_lcl_addr;
		    p += 4;
		    *p++ = v->uv_metric;
		    *p++ = v->uv_threshold;
		    *p++ = rflags;
		    ncount = p;
		    *p++ = 0;
		    datalen += 4 + 4;
		}
		*(u_int*)p = la->al_addr;
		p += 4;
		datalen += 4;
		(*ncount)++;
	    }
	}
    }
    if (datalen != 0)
	send_igmp(us, them, IGMP_DVMRP, DVMRP_NEIGHBORS2, htonl(MROUTED_LEVEL),
		  datalen);
}


/*
 * Process an incoming neighbor-list message.
 */
void accept_neighbors(src, dst, p, datalen, level)
    u_long src, dst, level;
    char *p;
    int datalen;
{
    log(LOG_INFO, 0, "ignoring spurious DVMRP neighbor list from %s to %s",
	inet_fmt(src, s1), inet_fmt(dst, s2));
}

/*
 * Process an incoming neighbor-list message.
 */
void accept_neighbors2(src, dst, p, datalen, level)
    u_long src, dst, level;
    char *p;
    int datalen;
{
    log(LOG_INFO, 0, "ignoring spurious DVMRP neighbor list2 from %s to %s",
	inet_fmt(src, s1), inet_fmt(dst, s2));
}


/*
 * Update the neighbor entry for neighbor 'addr' on vif 'vifi'.
 * 'msgtype' is the type of DVMRP message received from the neighbor.
 * Return TRUE if 'addr' is a valid neighbor, FALSE otherwise.
 */
int update_neighbor(vifi, addr, msgtype)
    vifi_t vifi;
    u_long addr;
    int msgtype;
{
    register struct uvif *v;
    register struct listaddr *n;

    v = &uvifs[vifi];

    /*
     * Confirm that 'addr' is a valid neighbor address on vif 'vifi'.
     * IT IS ASSUMED that this was preceded by a call to find_vif(), which
     * checks that 'addr' is either a valid remote tunnel endpoint or a
     * non-broadcast address belonging to a directly-connected subnet.
     * Therefore, here we check only that 'addr' is not our own address
     * (due to an impostor or erroneous loopback) or an address of the form
     * {subnet,0} ("the unknown host").  These checks are not performed in
     * find_vif() because those types of address are acceptable for some
     * types of IGMP message (such as group membership reports).
     */
    if (!(v->uv_flags & VIFF_TUNNEL) &&
	(addr == v->uv_lcl_addr ||
	 addr == v->uv_subnet )) {
	log(LOG_WARNING, 0,
	    "received DVMRP message from 'the unknown host' or self: %s",
	    inet_fmt(addr, s1));
	return (FALSE);
    }

    /*
     * If we have received a route report from a neighbor, and we believed
     * that we had no neighbors on this vif, send a full route report to
     * all neighbors on the vif.
     */

    if (msgtype == DVMRP_REPORT && v->uv_neighbors == NULL)
	report(ALL_ROUTES, vifi,
	       (v->uv_flags & VIFF_TUNNEL) ? addr : dvmrp_group);

    /*
     * Look for addr in list of neighbors; if found, reset its timer.
     */
    for (n = v->uv_neighbors; n != NULL; n = n->al_next) {
	if (addr == n->al_addr) {
	    n->al_timer = 0;
	    break;
	}
    }

    /*
     * If not found, add it to the list.  If the neighbor has a lower
     * IP address than me, yield querier duties to it.
     */
    if (n == NULL) {
	n = (struct listaddr *)malloc(sizeof(struct listaddr));
	if (n == NULL)
	    log(LOG_ERR, 0, "ran out of memory");    /* fatal */

	n->al_addr      = addr;
	n->al_timer     = 0;
	n->al_next      = v->uv_neighbors;
	v->uv_neighbors = n;

	if (!(v->uv_flags & VIFF_TUNNEL) &&
	    ntohl(addr) < ntohl(v->uv_lcl_addr))
	    v->uv_flags &= ~VIFF_QUERIER;
    }

    return (TRUE);
}


/*
 * On every timer interrupt, advance the timer in each neighbor and
 * group entry on every vif.
 */
void age_vifs()
{
    register vifi_t vifi;
    register struct uvif *v;
    register struct listaddr *a, *prev_a, *n;
    register u_long addr;

    for (vifi = 0, v = uvifs; vifi < numvifs; ++vifi, ++v ) {

	for (prev_a = (struct listaddr *)&(v->uv_neighbors),
	     a = v->uv_neighbors;
	     a != NULL;
	     prev_a = a, a = a->al_next) {

	    if ((a->al_timer += TIMER_INTERVAL) < NEIGHBOR_EXPIRE_TIME)
		continue;

	    /*
	     * Neighbor has expired; delete it from the neighbor list,
	     * delete it from the 'dominants' and 'subordinates arrays of
	     * any route entries and assume querier duties unless there is
	     * another neighbor with a lower IP address than mine.
	     */
	    addr = a->al_addr;
	    prev_a->al_next = a->al_next;
	    free((char *)a);
	    a = prev_a;

	    delete_neighbor_from_routes(addr, vifi);

	    if (!(v->uv_flags & VIFF_TUNNEL)) {
		v->uv_flags |= VIFF_QUERIER;
		for (n = v->uv_neighbors; n != NULL; n = n->al_next) {
		    if (ntohl(n->al_addr) < ntohl(v->uv_lcl_addr)) {
			v->uv_flags &= ~VIFF_QUERIER;
			break;
		    }
		}
	    }
	}

	for (prev_a = (struct listaddr *)&(v->uv_groups),
	     a = v->uv_groups;
	     a != NULL;
	     prev_a = a, a = a->al_next) {

	    if ((a->al_timer += TIMER_INTERVAL) < GROUP_EXPIRE_TIME)
		continue;

	    /*
	     * Group has expired; tell kernel and delete from group list.
	     */
	    k_del_group(vifi, a->al_addr);

	    prev_a->al_next = a->al_next;
	    free((char *)a);
	    a = prev_a;
	}
    }
}


/*
 * Print the contents of the uvifs array on file 'fp'.
 */
void dump_vifs(fp)
    FILE *fp;
{
    register vifi_t vifi;
    register struct uvif *v;
    register struct listaddr *a;

    fprintf(fp,
    "\nVirtual Interface Table\n%s",
    " Vif  Local-Address                           Metric  Thresh  Flags\n");

    for (vifi = 0, v = uvifs; vifi < numvifs; vifi++, v++) {

	fprintf(fp, " %2u   %-15s %6s: %-15s %4u %7u   ",
		vifi,
		inet_fmt(v->uv_lcl_addr, s1),
		(v->uv_flags & VIFF_TUNNEL) ?
			"tunnel":
			"subnet",
		(v->uv_flags & VIFF_TUNNEL) ?
			inet_fmt(v->uv_rmt_addr, s2) :
			inet_fmts(v->uv_subnet, v->uv_subnetmask, s3),
		v->uv_metric,
		v->uv_threshold);

	if (v->uv_flags & VIFF_DOWN)     fprintf(fp, " down");
	if (v->uv_flags & VIFF_DISABLED) fprintf(fp, " disabled");
	if (v->uv_flags & VIFF_QUERIER)  fprintf(fp, " querier");
	if (v->uv_flags & VIFF_SRCRT)    fprintf(fp, " src-rt");
	fprintf(fp, "\n");

	if (v->uv_neighbors != NULL) {
	    fprintf(fp, "                      peers : %-15s\n",
		    inet_fmt(v->uv_neighbors->al_addr, s1));
	    for (a = v->uv_neighbors->al_next; a != NULL; a = a->al_next) {
		fprintf(fp, "                              %-15s\n",
			inet_fmt(a->al_addr, s1));
	    }
	}

	if (v->uv_groups != NULL) {
	    fprintf(fp, "                      groups: %-15s\n",
		    inet_fmt(v->uv_groups->al_addr, s1));
	    for (a = v->uv_groups->al_next; a != NULL; a = a->al_next) {
		fprintf(fp, "                              %-15s\n",
			inet_fmt(a->al_addr, s1));
	    }
	}
    }
    fprintf(fp, "\n");
}
