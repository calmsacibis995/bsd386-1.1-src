divert(1)dnl
# Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
divert(0)
include(`../m4/cf.m4')dnl
VERSIONID(`BSDI $Id: bsdi.mc,v 1.1 1993/12/19 19:51:12 sanders Exp $')dnl
OSTYPE(`bsd4.4')dnl

# Sites not running DNS (aka named, bind) should remove the leading
# ``dnl'' from the following line.
dnl FEATURE(`nodns')

# If /bin/hostname is not a FQDN *and* you are not running a nameserver *and*
# the first (canonical) name in /etc/hosts for your machine is not the
# FQDN version then remove the leading ``dnl''s and fix your_domain_name.
dnl define(`NEED_DOMAIN', `1')
dnl DDyour_domain_name

# LOCAL_RELAY is a site that will handle unqualified names, this is
# basically for site/company/department wide alias forwarding.  By
# default mail is delivered on the local host.  If you want to use
# this then fix your_local_relay and remove the leading ``dnl''.
dnl define(`LOCAL_RELAY', `your_local_relay')dnl

# Relay for fake domains (.UUCP .BITNET .CSNET).
# To enable, remove the leading ``dnl'' and fix your_relay_host:
dnl define(`UUCP_RELAY', `your_relay_host')dnl
dnl define(`BITNET_RELAY', `your_relay_host')dnl
dnl define(`CSNET_RELAY', `your_relay_host')dnl

# `MASQUERADE_AS' is used to hide behind a gateway.  If you wish to
# do that then fix masquerade_as, remove the leading ``dnl'' and
# add any accounts you wish to be exposed (i.e., not hidden) to the
# `EXPOSED_USER' list.
dnl MASQUERADE_AS(`masquerade_as')
EXPOSED_USER(`postmaster hostmaster webmaster')

# If you are setting up a UUCP connection then do not ``define'' a UUCP_RELAY.
# You will need to create # a siteconfig file (siteconfig/site_config_file.m4)
# that contains a list of connected hosts using the `SITE(otherhost)' macro.
#
# `SITECONFIG(site_config_file, name_of_site, connection)'
# site_config_file is a file in the cf/siteconfig directory (less the `.m4')
# name_of_site     is the UUCP name of your site
# connection       is one of U, W, X, or Y; where U means the sites listed
#                  in the config file are connected locally;  W, X, and Y
#                  build remote UUCP hub classes (e.g., `$=W').
#
dnl SITECONFIG(`uucp.yoursite', `your_uucp_name', U)

MAILER(local)
MAILER(smtp)
MAILER(uucp)
