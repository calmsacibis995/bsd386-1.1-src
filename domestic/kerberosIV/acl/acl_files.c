/*
 *	$Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/acl/acl_files.c,v $
 *	$Author: kolstad $
 *
 * Copyright 1987,1989 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 */

#ifndef lint
static char rcsid_acl_files_c[] = "$Id: acl_files.c,v 1.2 1992/01/04 18:35:19 kolstad Exp $";
#endif lint


/*** Routines for manipulating access control list files ***/

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <ctype.h>
#include "des.h"
#include "krb.h"

#ifndef KRB_REALM
#define KRB_REALM	"CS.BERKELEY.EDU"
#endif

/* "aname.inst@realm" */
#define MAX_PRINCIPAL_SIZE  (ANAME_SZ + INST_SZ + REALM_SZ + 3)
#define INST_SEP '.'
#define REALM_SEP '@'

#define LINESIZE 2048		/* Maximum line length in an acl file */

#define NEW_FILE "%s.~NEWACL~"	/* Format for name of altered acl file */
#define WAIT_TIME 300		/* Maximum time allowed write acl file */

#define CACHED_ACLS 8		/* How many acls to cache */
				/* Each acl costs 1 open file descriptor */
#define ACL_LEN 16		/* Twice a reasonable acl length */

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define COR(a,b) ((a!=NULL)?(a):(b))

extern int errno;

extern char *malloc(), *calloc();
extern time_t time();

/* Canonicalize a principal name */
/* If instance is missing, it becomes "" */
/* If realm is missing, it becomes the local realm */
/* Canonicalized form is put in canon, which must be big enough to hold
   MAX_PRINCIPAL_SIZE characters */
acl_canonicalize_principal(principal, canon)
char *principal;
char *canon;
{
    char *dot, *atsign, *end;
    int len;

    dot = index(principal, INST_SEP);
    atsign = index(principal, REALM_SEP);

    /* Maybe we're done already */
    if(dot != NULL && atsign != NULL) {
	if(dot < atsign) {
	    /* It's for real */
	    /* Copy into canon */
	    strncpy(canon, principal, MAX_PRINCIPAL_SIZE);
	    canon[MAX_PRINCIPAL_SIZE-1] = '\0';
	    return;
	} else {
	    /* Nope, it's part of the realm */
	    dot = NULL;
	}
    }
    
    /* No such luck */
    end = principal + strlen(principal);

    /* Get the principal name */
    len = MIN(ANAME_SZ, COR(dot, COR(atsign, end)) - principal);
    strncpy(canon, principal, len);
    canon += len;

    /* Add INST_SEP */
    *canon++ = INST_SEP;

    /* Get the instance, if it exists */
    if(dot != NULL) {
	++dot;
	len = MIN(INST_SZ, COR(atsign, end) - dot);
	strncpy(canon, dot, len);
	canon += len;
    }

    /* Add REALM_SEP */
    *canon++ = REALM_SEP;

    /* Get the realm, if it exists */
    /* Otherwise, default to local realm */
    if(atsign != NULL) {
	++atsign;
	len = MIN(REALM_SZ, end - atsign);
	strncpy(canon, atsign, len);
	canon += len;
	*canon++ = '\0';
    } else if(krb_get_lrealm(canon, 1) != KSUCCESS) {
	strcpy(canon, KRB_REALM);
    }
}
	    
/* Get a lock to modify acl_file */
/* Return new FILE pointer */
/* or NULL if file cannot be modified */
/* REQUIRES WRITE PERMISSION TO CONTAINING DIRECTORY */
static FILE *acl_lock_file(acl_file)
char *acl_file;
{
    struct stat s;
    char new[LINESIZE];
    int nfd;
    FILE *nf;
    int mode;

    if(stat(acl_file, &s) < 0) return(NULL);
    mode = s.st_mode;
    sprintf(new, NEW_FILE, acl_file);
    for(;;) {
	/* Open the new file */
	if((nfd = open(new, O_WRONLY|O_CREAT|O_EXCL, mode)) < 0) {
	    if(errno == EEXIST) {
		/* Maybe somebody got here already, maybe it's just old */
		if(stat(new, &s) < 0) return(NULL);
		if(time(0) - s.st_ctime > WAIT_TIME) {
		    /* File is stale, kill it */
		    unlink(new);
		    continue;
		} else {
		    /* Wait and try again */
		    sleep(1);
		    continue;
		}
	    } else {
		/* Some other error, we lose */
		return(NULL);
	    }
	}

	/* If we got to here, the lock file is ours and ok */
	/* Reopen it under stdio */
	if((nf = fdopen(nfd, "w")) == NULL) {
	    /* Oops, clean up */
	    unlink(new);
	}
	return(nf);
    }
}

static int acl_abort();

/* Commit changes to acl_file written onto FILE *f */
/* Returns zero if successful */
/* Returns > 0 if lock was broken */
/* Returns < 0 if some other error occurs */
/* Closes f */
static int acl_commit(acl_file, f)
char *acl_file;
FILE *f;     
{
    char new[LINESIZE];
    int ret;
    struct stat s;

    sprintf(new, NEW_FILE, acl_file);
    if(fflush(f) < 0
       || fstat(fileno(f), &s) < 0
       || s.st_nlink == 0) {
	acl_abort(acl_file, f);
	return(-1);
    }

    ret = rename(new, acl_file);
    fclose(f);
    return(ret);
}

/* Abort changes to acl_file written onto FILE *f */
/* Returns 0 if successful, < 0 otherwise */
/* Closes f */
static int acl_abort(acl_file, f)
char *acl_file;
FILE *f;     
{
    char new[LINESIZE];
    int ret;
    struct stat s;

    /* make sure we aren't nuking someone else's file */
    if(fstat(fileno(f), &s) < 0
       || s.st_nlink == 0) {
	   fclose(f);
	   return(-1);
       } else {
	   sprintf(new, NEW_FILE, acl_file);
	   ret = unlink(new);
	   fclose(f);
	   return(ret);
       }
}

/* Initialize an acl_file */
/* Creates the file with permissions perm if it does not exist */
/* Erases it if it does */
/* Returns return value of acl_commit */
int acl_initialize(acl_file, perm)
char *acl_file;
int perm;
{
    FILE *new;
    int fd;

    /* Check if the file exists already */
    if((new = acl_lock_file(acl_file)) != NULL) {
	return(acl_commit(acl_file, new));
    } else {
	/* File must be readable and writable by owner */
	if((fd = open(acl_file, O_CREAT|O_EXCL, perm|0600)) < 0) {
	    return(-1);
	} else {
	    close(fd);
	    return(0);
	}
    }
}

/* Eliminate all whitespace character in buf */
/* Modifies its argument */
static nuke_whitespace(buf)
char *buf;
{
    register char *pin, *pout;

    for(pin = pout = buf; *pin != '\0'; pin++)
	if(!isspace(*pin)) *pout++ = *pin;
    *pout = '\0';		/* Terminate the string */
}

/* Hash table stuff */

struct hashtbl {
    int size;			/* Max number of entries */
    int entries;		/* Actual number of entries */
    char **tbl;			/* Pointer to start of table */
};

/* Make an empty hash table of size s */
static struct hashtbl *make_hash(size)
int size;
{
    struct hashtbl *h;

    if(size < 1) size = 1;
    h = (struct hashtbl *) malloc(sizeof(struct hashtbl));
    h->size = size;
    h->entries = 0;
    h->tbl = (char **) calloc(size, sizeof(char *));
    return(h);
}

/* Destroy a hash table */
static destroy_hash(h)
struct hashtbl *h;
{
    int i;

    for(i = 0; i < h->size; i++) {
	if(h->tbl[i] != NULL) free(h->tbl[i]);
    }
    free(h->tbl);
    free(h);
}

/* Compute hash value for a string */
static unsigned hashval(s)
register char *s;
{
    register unsigned hv;

    for(hv = 0; *s != '\0'; s++) {
	hv ^= ((hv << 3) ^ *s);
    }
    return(hv);
}

/* Add an element to a hash table */
static add_hash(h, el)
struct hashtbl *h;
char *el;
{
    unsigned hv;
    char *s;
    char **old;
    int i;

    /* Make space if it isn't there already */
    if(h->entries + 1 > (h->size >> 1)) {
	old = h->tbl;
	h->tbl = (char **) calloc(h->size << 1, sizeof(char *));
	for(i = 0; i < h->size; i++) {
	    if(old[i] != NULL) {
		hv = hashval(old[i]) % (h->size << 1);
		while(h->tbl[hv] != NULL) hv = (hv+1) % (h->size << 1);
		h->tbl[hv] = old[i];
	    }
	}
	h->size = h->size << 1;
	free(old);
    }

    hv = hashval(el) % h->size;
    while(h->tbl[hv] != NULL && strcmp(h->tbl[hv], el)) hv = (hv+1) % h->size;
    s = malloc(strlen(el)+1);
    strcpy(s, el);
    h->tbl[hv] = s;
    h->entries++;
}

/* Returns nonzero if el is in h */
static check_hash(h, el)
struct hashtbl *h;
char *el;
{
    unsigned hv;

    for(hv = hashval(el) % h->size;
	h->tbl[hv] != NULL;
	hv = (hv + 1) % h->size) {
	    if(!strcmp(h->tbl[hv], el)) return(1);
	}
    return(0);
}

struct acl {
    char filename[LINESIZE];	/* Name of acl file */
    int fd;			/* File descriptor for acl file */
    struct stat status;		/* File status at last read */
    struct hashtbl *acl;	/* Acl entries */
};

static struct acl acl_cache[CACHED_ACLS];

static int acl_cache_count = 0;
static int acl_cache_next = 0;

/* Returns < 0 if unsuccessful in loading acl */
/* Returns index into acl_cache otherwise */
/* Note that if acl is already loaded, this is just a lookup */
static int acl_load(name)
char *name;
{
    int i;
    FILE *f;
    struct stat s;
    char buf[MAX_PRINCIPAL_SIZE];
    char canon[MAX_PRINCIPAL_SIZE];

    /* See if it's there already */
    for(i = 0; i < acl_cache_count; i++) {
	if(!strcmp(acl_cache[i].filename, name)
	   && acl_cache[i].fd >= 0) goto got_it;
    }

    /* It isn't, load it in */
    /* maybe there's still room */
    if(acl_cache_count < CACHED_ACLS) {
	i = acl_cache_count++;
    } else {
	/* No room, clean one out */
	i = acl_cache_next;
	acl_cache_next = (acl_cache_next + 1) % CACHED_ACLS;
	close(acl_cache[i].fd);
	if(acl_cache[i].acl) {
	    destroy_hash(acl_cache[i].acl);
	    acl_cache[i].acl = (struct hashtbl *) 0;
	}
    }

    /* Set up the acl */
    strcpy(acl_cache[i].filename, name);
    if((acl_cache[i].fd = open(name, O_RDONLY, 0)) < 0) return(-1);
    /* Force reload */
    acl_cache[i].acl = (struct hashtbl *) 0;

 got_it:
    /*
     * See if the stat matches
     *
     * Use stat(), not fstat(), as the file may have been re-created by
     * acl_add or acl_delete.  If this happens, the old inode will have
     * no changes in the mod-time and the following test will fail.
     */
    if(stat(acl_cache[i].filename, &s) < 0) return(-1);
    if(acl_cache[i].acl == (struct hashtbl *) 0
       || s.st_nlink != acl_cache[i].status.st_nlink
       || s.st_mtime != acl_cache[i].status.st_mtime
       || s.st_ctime != acl_cache[i].status.st_ctime) {
	   /* Gotta reload */
	   if(acl_cache[i].fd >= 0) close(acl_cache[i].fd);
	   if((acl_cache[i].fd = open(name, O_RDONLY, 0)) < 0) return(-1);
	   if((f = fdopen(acl_cache[i].fd, "r")) == NULL) return(-1);
	   if(acl_cache[i].acl) destroy_hash(acl_cache[i].acl);
	   acl_cache[i].acl = make_hash(ACL_LEN);
	   while(fgets(buf, sizeof(buf), f) != NULL) {
	       nuke_whitespace(buf);
	       acl_canonicalize_principal(buf, canon);
	       add_hash(acl_cache[i].acl, canon);
	   }
	   fclose(f);
	   acl_cache[i].status = s;
       }
    return(i);
}

/* Returns nonzero if it can be determined that acl contains principal */
/* Principal is not canonicalized, and no wildcarding is done */
acl_exact_match(acl, principal)
char *acl;
char *principal;
{
    int idx;

    return((idx = acl_load(acl)) >= 0
	   && check_hash(acl_cache[idx].acl, principal));
}

/* Returns nonzero if it can be determined that acl contains principal */
/* Recognizes wildcards in acl of the form
   name.*@realm, *.*@realm, and *.*@* */
acl_check(acl, principal)
char *acl;
char *principal;
{
    char buf[MAX_PRINCIPAL_SIZE];
    char canon[MAX_PRINCIPAL_SIZE];
    char *realm;

    acl_canonicalize_principal(principal, canon);

    /* Is it there? */
    if(acl_exact_match(acl, canon)) return(1);

    /* Try the wildcards */
    realm = index(canon, REALM_SEP);
    *index(canon, INST_SEP) = '\0';	/* Chuck the instance */

    sprintf(buf, "%s.*%s", canon, realm);
    if(acl_exact_match(acl, buf)) return(1);

    sprintf(buf, "*.*%s", realm);
    if(acl_exact_match(acl, buf) || acl_exact_match(acl, "*.*@*")) return(1);
       
    return(0);
}

/* Adds principal to acl */
/* Wildcards are interpreted literally */
acl_add(acl, principal)
char *acl;
char *principal;
{
    int idx;
    int i;
    FILE *new;
    char canon[MAX_PRINCIPAL_SIZE];

    acl_canonicalize_principal(principal, canon);

    if((new = acl_lock_file(acl)) == NULL) return(-1);
    if((acl_exact_match(acl, canon))
       || (idx = acl_load(acl)) < 0) {
	   acl_abort(acl, new);
	   return(-1);
       }
    /* It isn't there yet, copy the file and put it in */
    for(i = 0; i < acl_cache[idx].acl->size; i++) {
	if(acl_cache[idx].acl->tbl[i] != NULL) {
	    if(fputs(acl_cache[idx].acl->tbl[i], new) == NULL
	       || putc('\n', new) != '\n') {
		   acl_abort(acl, new);
		   return(-1);
	       }
	}
    }
    fputs(canon, new);
    putc('\n', new);
    return(acl_commit(acl, new));
}

/* Removes principal from acl */
/* Wildcards are interpreted literally */
acl_delete(acl, principal)
char *acl;
char *principal;
{
    int idx;
    int i;
    FILE *new;
    char canon[MAX_PRINCIPAL_SIZE];

    acl_canonicalize_principal(principal, canon);

    if((new = acl_lock_file(acl)) == NULL) return(-1);
    if((!acl_exact_match(acl, canon))
       || (idx = acl_load(acl)) < 0) {
	   acl_abort(acl, new);
	   return(-1);
       }
    /* It isn't there yet, copy the file and put it in */
    for(i = 0; i < acl_cache[idx].acl->size; i++) {
	if(acl_cache[idx].acl->tbl[i] != NULL
	   && strcmp(acl_cache[idx].acl->tbl[i], canon)) {
	       fputs(acl_cache[idx].acl->tbl[i], new);
	       putc('\n', new);
	}
    }
    return(acl_commit(acl, new));
}

