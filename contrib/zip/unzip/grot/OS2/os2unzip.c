/* Unix/HPFS filename translation for FAT file systems */
/*  (with special unzip modifications:  sflag) */

/* also includes lots of EA code for OS/2 */

/* Author: Kai Uwe Rommel */

#include "unzip.h"

#define INCL_NOPM
#define INCL_DOSNLS
#define INCL_DOSERRORS
#define ULONG _ULONG
#include <os2.h>
#undef ULONG

#ifdef __WATCOMC__
unsigned char __near _osmode = OS2_MODE;
#endif


#define EAID     0x0009


extern int tflag, quietflg;


typedef struct
{
  USHORT nID;
  USHORT nSize;
  ULONG lSize;
}
EAHEADER, *PEAHEADER;


#ifndef __32BIT__

typedef struct
{
  ULONG oNextEntryOffset;
  BYTE fEA;
  BYTE cbName;
  USHORT cbValue;
  CHAR szName[1];
}
FEA2, *PFEA2;

typedef struct
{
  ULONG cbList;
  FEA2 list[1];
}
FEA2LIST, *PFEA2LIST;

#endif


#ifndef __32BIT__
#define DosSetPathInfo(p1, p2, p3, p4, p5) \
        DosSetPathInfo(p1, p2, p3, p4, p5, 0)
#define DosQueryPathInfo(p1, p2, p3, p4) \
	DosQPathInfo(p1, p2, p3, p4, 0)
#define DosMapCase DosCaseMap
#define DosQueryCtryInfo DosGetCtryInfo
#endif


#ifndef ZIPINFO


extern int sflag;  /* user wants to allow spaces (e.g., "EA DATA. SF") */

void ChangeNameForFAT(char *name)
{
  char *src, *dst, *next, *ptr, *dot, *start;
  static char invalid[] = ":;,=+\"[]<>| \t";

  if ( isalpha(name[0]) && (name[1] == ':') )
    start = name + 2;
  else
    start = name;

  src = dst = start;
  if ( (*src == '/') || (*src == '\\') )
    src++, dst++;

  while ( *src )
  {
    for ( next = src; *next && (*next != '/') && (*next != '\\'); next++ );

    for ( ptr = src, dot = NULL; ptr < next; ptr++ )
      if ( *ptr == '.' )
      {
        dot = ptr; /* remember last dot */
        *ptr = '_';
      }

    if ( dot == NULL )
      for ( ptr = src; ptr < next; ptr++ )
        if ( *ptr == '_' )
          dot = ptr; /* remember last _ as if it were a dot */

    if ( dot && (dot > src) &&
         ((next - dot <= 4) ||
          ((next - src > 8) && (dot - src > 3))) )
    {
      if ( dot )
        *dot = '.';

      for ( ptr = src; (ptr < dot) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;

      for ( ptr = dot; (ptr < next) && ((ptr - dot) < 4); ptr++ )
        *dst++ = *ptr;
    }
    else
    {
      if ( dot && (next - src == 1) )
        *dot = '.';           /* special case: "." as a path component */

      for ( ptr = src; (ptr < next) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;
    }

    *dst++ = *next; /* either '/' or 0 */

    if ( *next )
    {
      src = next + 1;

      if ( *src == 0 ) /* handle trailing '/' on dirs ! */
        *dst = 0;
    }
    else
      break;
  }

  for ( src = start; *src != 0; ++src )
    if ( (strchr(invalid, *src) != NULL) ||
         ((*src == ' ') && !sflag) )  /* allow spaces if user wants */
        *src = '_';
}


int IsFileNameValid(char *name)
{
  HFILE hf;
#ifdef __32BIT__
  ULONG uAction;
#else
  USHORT uAction;
#endif

  switch( DosOpen(name, &hf, &uAction, 0, 0, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0) )
  {
  case ERROR_INVALID_NAME:
  case ERROR_FILENAME_EXCED_RANGE:
    return FALSE;
  case NO_ERROR:
    DosClose(hf);
  default:
    return TRUE;
  }
}


int GetCountryInfo(void)
{
    COUNTRYINFO ctryi;
    COUNTRYCODE ctryc;
#ifdef __32BIT__
    ULONG cbInfo;
#else
    USHORT cbInfo;
#endif

  ctryc.country = ctryc.codepage = 0;

  if ( DosQueryCtryInfo(sizeof(ctryi), &ctryc, &ctryi, &cbInfo) != NO_ERROR )
    return 0;

  return ctryi.fsDateFmt;
}


long GetFileTime(char *name)
{
#ifdef __32BIT__
  FILESTATUS3 fs;
#else
  FILESTATUS fs;
#endif
  USHORT nDate, nTime;

  if ( DosQueryPathInfo(name, 1, (PBYTE) &fs, sizeof(fs)) )
    return -1;

  nDate = * (USHORT *) &fs.fdateLastWrite;
  nTime = * (USHORT *) &fs.ftimeLastWrite;

  return ((ULONG) nDate) << 16 | nTime;
}


void SetPathInfo(char *path, UWORD moddate, UWORD modtime, int flags)
{
  union {
    FDATE fd;               /* system file date record */
    UWORD zdate;            /* date word */
  } ud;
  union {
    FTIME ft;               /* system file time record */
    UWORD ztime;            /* time word */
  } ut;
  FILESTATUS fs;
  USHORT nLength;
  char szName[CCHMAXPATH];

  strcpy(szName, path);
  nLength = strlen(szName);
  if (szName[nLength - 1] == '/')
    szName[nLength - 1] = 0;

  if ( DosQueryPathInfo(szName, FIL_STANDARD, (PBYTE) &fs, sizeof(fs)) )
    return;

  ud.zdate = moddate;
  ut.ztime = modtime;
  fs.fdateLastWrite = fs.fdateCreation = ud.fd;
  fs.ftimeLastWrite = fs.ftimeCreation = ut.ft;

  if ( flags != -1 )
    fs.attrFile = flags; /* hidden, system, archive, read-only */

  DosSetPathInfo(szName, FIL_STANDARD, (PBYTE) &fs, sizeof(fs), 0);
}


typedef struct
{
  ULONG cbList;               /* length of value + 22 */
#ifdef __32BIT__
  ULONG oNext;
#endif
  BYTE fEA;                   /* 0 */
  BYTE cbName;                /* length of ".LONGNAME" = 9 */
  USHORT cbValue;             /* length of value + 4 */
  BYTE szName[10];            /* ".LONGNAME" */
  USHORT eaType;              /* 0xFFFD for length-preceded ASCII */
  USHORT eaSize;              /* length of value */
  BYTE szValue[CCHMAXPATH];
}
FEALST;


int SetLongNameEA(char *name, char *longname)
{
  EAOP eaop;
  FEALST fealst;

  if ( _osmode == DOS_MODE )
    return 0;

  eaop.fpFEAList = (PFEALIST) &fealst;
  eaop.fpGEAList = NULL;
  eaop.oError = 0;

  strcpy(fealst.szName, ".LONGNAME");
  strcpy(fealst.szValue, longname);

  fealst.cbList  = sizeof(fealst) - CCHMAXPATH + strlen(fealst.szValue);
  fealst.cbName  = (BYTE) strlen(fealst.szName);
  fealst.cbValue = sizeof(USHORT) * 2 + strlen(fealst.szValue);

#ifdef __32BIT__
  fealst.oNext   = 0;
#endif
  fealst.fEA     = 0;
  fealst.eaType  = 0xFFFD;
  fealst.eaSize  = strlen(fealst.szValue);

  return DosSetPathInfo(name, FIL_QUERYEASIZE,
                        (PBYTE) &eaop, sizeof(eaop), 0);
}


int IsEA(void *extra_field)
{
  EAHEADER *pEAblock = (PEAHEADER) extra_field;
  return extra_field != NULL && pEAblock -> nID == EAID;
}


void SetEAs(char *path, void *eablock)
{
  EAHEADER *pEAblock = (PEAHEADER) eablock;
#ifdef __32BIT__
  EAOP2 eaop;
  PFEA2LIST pFEA2list;
#else
  EAOP eaop;
  PFEALIST pFEAlist;
  PFEA pFEA;
  PFEA2LIST pFEA2list;
  PFEA2 pFEA2;
  ULONG nLength2;
#endif
  USHORT nLength;
  char szName[CCHMAXPATH];

  if ( !IsEA(eablock) )
    return;

  if ( _osmode == DOS_MODE )
    return;

  strcpy(szName, path);
  nLength = strlen(szName);
  if (szName[nLength - 1] == '/')
    szName[nLength - 1] = 0;

  if ( (pFEA2list = (PFEA2LIST) malloc((size_t) pEAblock -> lSize)) == NULL )
    return;

  if ( memextract((char *) pFEA2list, pEAblock -> lSize,
	          (char *) (pEAblock + 1), 
		  pEAblock -> nSize - sizeof(pEAblock -> lSize)) )
  {
    free(pFEA2list);
    return;
  }

#ifdef __32BIT__
  eaop.fpGEA2List = NULL;
  eaop.fpFEA2List = pFEA2list;
#else
  pFEAlist  = (PVOID) pFEA2list;
  pFEA2 = pFEA2list -> list;
  pFEA  = pFEAlist  -> list;

  do
  {
    nLength2 = pFEA2 -> oNextEntryOffset;
    nLength = sizeof(FEA) + pFEA2 -> cbName + 1 + pFEA2 -> cbValue;

    memcpy(pFEA, (PCH) pFEA2 + sizeof(pFEA2 -> oNextEntryOffset), nLength);

    pFEA2 = (PFEA2) ((PCH) pFEA2 + nLength2);
    pFEA = (PFEA) ((PCH) pFEA + nLength);
  }
  while ( nLength2 != 0 );

  pFEAlist -> cbList = (PCH) pFEA - (PCH) pFEAlist;

  eaop.fpGEAList = NULL;
  eaop.fpFEAList = pFEAlist;
#endif

  eaop.oError = 0;
  DosSetPathInfo(szName, FIL_QUERYEASIZE, (PBYTE) &eaop, sizeof(eaop), 0);

  if (!tflag && (quietflg < 2))
    printf(" (%ld bytes EA's)", pFEA2list -> cbList);

  free(pFEA2list);
}


ULONG SizeOfEAs(void *extra_field)
{
  EAHEADER *pEAblock = (PEAHEADER) extra_field;

  if ( extra_field != NULL && pEAblock -> nID == EAID )
    return pEAblock -> lSize;

  return 0;
}


#endif /* !ZIPINFO */


static unsigned char cUpperCase[256], cLowerCase[256];
static BOOL bInitialized;

static void InitNLS(void)
{
  unsigned nCnt, nU;
  COUNTRYCODE cc;

  bInitialized = TRUE;

  for ( nCnt = 0; nCnt < 256; nCnt++ )
    cUpperCase[nCnt] = cLowerCase[nCnt] = (unsigned char) nCnt;

  cc.country = cc.codepage = 0;
  DosMapCase(sizeof(cUpperCase), &cc, (PCHAR) cUpperCase);

  for ( nCnt = 0; nCnt < 256; nCnt++ )
  {
    nU = cUpperCase[nCnt];
    if (nU != nCnt && cLowerCase[nU] == (unsigned char) nU)
      cLowerCase[nU] = (unsigned char) nCnt;
  }

  for ( nCnt = 'A'; nCnt <= 'Z'; nCnt++ )
    cLowerCase[nCnt] = (unsigned char) (nCnt - 'A' + 'a');
}


int IsUpperNLS(int nChr)
{
  if (!bInitialized)
    InitNLS();
  return (cUpperCase[nChr] == (unsigned char) nChr);
}


int ToLowerNLS(int nChr)
{
  if (!bInitialized)
    InitNLS();
  return cLowerCase[nChr];
}
