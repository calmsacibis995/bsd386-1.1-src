
/*
** Distributed with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

extern int pfcode;

#define PRF_STATS	0x0001
#define PRF_CMD		0x0008
#define PRF_CLASS       0x0004

#define PRF_QUES	0x0010
#define PRF_ANS		0x0020
#define PRF_AUTH	0x0040
#define PRF_ADD		0x0080

#define PRF_HEAD1	0x0100
#define PRF_HEAD2	0x0200
#define PRF_TTLID	0x0400
#define PRF_HEADX	0x0800

#define PRF_QUERY	0x1000
#define PRF_REPLY	0x2000
#define PRF_INIT        0x4000
#define PRF_SORT        0x8000

#define PRF_DEF		0x2ff9
#define PRF_MIN		0xA930
#define PRF_ZONE        0x24f9

