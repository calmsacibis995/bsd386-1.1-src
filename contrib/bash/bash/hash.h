/* hash.h -- the data structures used in hashing in Bash. */

#if !defined (_HASH_H_)
#define _HASH_H_

typedef struct bucket_contents {
  struct bucket_contents *next;	/* Link to next hashed key in this bucket. */
  char *key;			/* What we look up. */
  char *data;			/* What we really want. */
  int times_found;		/* Number of times this item has been found. */
} BUCKET_CONTENTS;

typedef struct hash_table {
  BUCKET_CONTENTS **bucket_array;	/* Where the data is kept. */
  int nbuckets;			/* How many buckets does this table have. */
  int nentries;			/* How many entries does this table have. */
} HASH_TABLE;

extern BUCKET_CONTENTS
  *find_hash_item (), *remove_hash_item (), *add_hash_item (),
  *get_hash_bucket ();

/* Redefine the function as a macro for speed. */
#define get_hash_bucket(bucket, table) \
	((bucket < table->nbuckets) ?  \
	table->bucket_array[bucket] : \
	(BUCKET_CONTENTS *)NULL)

extern int hash_string ();
extern HASH_TABLE *make_hash_table ();

/* Default number of buckets in the hash table. */
#define DEFAULT_HASH_BUCKETS 107

#if !defined (NULL)
#  if defined (__STDC__)
#    define NULL ((void *) 0)
#  else
#    define NULL 0x0
#  endif /* !__STDC__ */
#endif /* !NULL */

#endif /* _HASH_H */
