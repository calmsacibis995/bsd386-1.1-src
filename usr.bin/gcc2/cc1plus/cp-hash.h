/* C code produced by gperf version 2.5 (GNU C++ version) */
/* Command-line: gperf -p -j1 -g -o -t -N is_reserved_word -k1,4,7,$ ../../devo/gcc/gplus.gperf  */
/* Command-line: gperf -p -j1 -g -o -t -N is_reserved_word -k1,4,$,7 gplus.gperf  */
struct resword { char *name; short token; enum rid rid;};

#define TOTAL_KEYWORDS 84
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 154
/* maximum key range = 151, duplicates = 0 */

#ifdef __GNUC__
inline
#endif
static unsigned int
hash (str, len)
     register char *str;
     register int unsigned len;
{
  static unsigned char asso_values[] =
    {
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
     155, 155, 155, 155, 155,   0, 155,  41,   1,  52,
      25,   0,  14,  43,  28,  60, 155,   4,  19,   4,
      13,   1,  37, 155,  20,   0,   4,  61,  48,  30,
       1,  67, 155, 155, 155, 155, 155, 155,
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 7:
        hval += asso_values[str[6]];
      case 6:
      case 5:
      case 4:
        hval += asso_values[str[3]];
      case 3:
      case 2:
      case 1:
        hval += asso_values[str[0]];
    }
  return hval + asso_values[str[len - 1]];
}

#ifdef __GNUC__
inline
#endif
struct resword *
is_reserved_word (str, len)
     register char *str;
     register unsigned int len;
{
  static struct resword wordlist[] =
    {
      {"",}, {"",}, {"",}, {"",}, 
      {"else",  ELSE, NORID,},
      {"",}, {"",}, 
      {"__asm__",  GCC_ASM_KEYWORD, NORID},
      {"this",  THIS, NORID,},
      {"__asm",  GCC_ASM_KEYWORD, NORID},
      {"except",  EXCEPT, NORID		/* Extension */,},
      {"__headof__",  HEADOF, NORID},
      {"enum",  ENUM, NORID,},
      {"",}, 
      {"__const__",  TYPE_QUAL, RID_CONST},
      {"__volatile",  TYPE_QUAL, RID_VOLATILE},
      {"__const",  TYPE_QUAL, RID_CONST},
      {"__volatile__",  TYPE_QUAL, RID_VOLATILE},
      {"",}, 
      {"extern",  SCSPEC, RID_EXTERN,},
      {"sizeof",  SIZEOF, NORID,},
      {"",}, {"",}, 
      {"__headof",  HEADOF, NORID},
      {"typeof",  TYPEOF, NORID,},
      {"raise",  RAISE, NORID		/* Extension */,},
      {"raises",  RAISES, NORID		/* Extension */,},
      {"__extension__",  EXTENSION, NORID},
      {"do",  DO, NORID,},
      {"short",  TYPESPEC, RID_SHORT,},
      {"__classof__",  CLASSOF, NORID},
      {"delete",  DELETE, NORID,},
      {"double",  TYPESPEC, RID_DOUBLE,},
      {"",}, 
      {"__inline",  SCSPEC, RID_INLINE},
      {"typeid",  TYPEID, NORID,},
      {"__inline__",  SCSPEC, RID_INLINE},
      {"for",  FOR, NORID,},
      {"switch",  SWITCH, NORID,},
      {"typedef",  SCSPEC, RID_TYPEDEF,},
      {"throw",  THROW, NORID		/* Extension */,},
      {"",}, 
      {"__classof",  CLASSOF, NORID},
      {"__alignof__",  ALIGNOF, NORID},
      {"signed",  TYPESPEC, RID_SIGNED,},
      {"friend",  SCSPEC, RID_FRIEND,},
      {"new",  NEW, NORID,},
      {"auto",  SCSPEC, RID_AUTO,},
      {"asm",  ASM_KEYWORD, NORID,},
      {"goto",  GOTO, NORID,},
      {"operator",  OPERATOR, NORID,},
      {"break",  BREAK, NORID,},
      {"mutable",  SCSPEC, RID_MUTABLE,},
      {"template",  TEMPLATE, NORID,},
      {"while",  WHILE, NORID,},
      {"__alignof",  ALIGNOF, NORID},
      {"case",  CASE, NORID,},
      {"class",  AGGR, RID_CLASS,},
      {"",}, {"",}, {"",}, 
      {"const",  TYPE_QUAL, RID_CONST,},
      {"static",  SCSPEC, RID_STATIC,},
      {"all",  ALL, NORID			/* Extension */,},
      {"float",  TYPESPEC, RID_FLOAT,},
      {"",}, {"",}, 
      {"int",  TYPESPEC, RID_INT,},
      {"reraise",  RERAISE, NORID		/* Extension */,},
      {"__label__",  LABEL, NORID},
      {"__signed__",  TYPESPEC, RID_SIGNED},
      {"struct",  AGGR, RID_RECORD,},
      {"",}, 
      {"headof",  HEADOF, NORID,},
      {"try",  TRY, NORID			/* Extension */,},
      {"__attribute",  ATTRIBUTE, NORID},
      {"if",  IF, NORID,},
      {"__attribute__",  ATTRIBUTE, NORID},
      {"__typeof__",  TYPEOF, NORID},
      {"protected",  VISSPEC, RID_PROTECTED,},
      {"union",  AGGR, RID_UNION,},
      {"default",  DEFAULT, NORID,},
      {"exception",  AGGR, RID_EXCEPTION	/* Extension */,},
      {"",}, {"",}, 
      {"__wchar_t",  TYPESPEC, RID_WCHAR  /* Unique to ANSI C++ */,},
      {"",}, 
      {"classof",  CLASSOF, NORID,},
      {"",}, {"",}, 
      {"__typeof",  TYPEOF, NORID},
      {"",}, 
      {"private",  VISSPEC, RID_PRIVATE,},
      {"__signed",  TYPESPEC, RID_SIGNED},
      {"",}, 
      {"overload",  OVERLOAD, NORID,},
      {"char",  TYPESPEC, RID_CHAR,},
      {"virtual",  SCSPEC, RID_VIRTUAL,},
      {"",}, {"",}, 
      {"return",  RETURN, NORID,},
      {"",}, 
      {"void",  TYPESPEC, RID_VOID,},
      {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"register",  SCSPEC, RID_REGISTER,},
      {"long",  TYPESPEC, RID_LONG,},
      {"",}, {"",}, {"",}, {"",}, 
      {"public",  VISSPEC, RID_PUBLIC,},
      {"",}, 
      {"volatile",  TYPE_QUAL, RID_VOLATILE,},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"continue",  CONTINUE, NORID,},
      {"inline",  SCSPEC, RID_INLINE,},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"dynamic_cast",  DYNAMIC_CAST, NORID,},
      {"",}, {"",}, 
      {"catch",  CATCH, NORID,},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"unsigned",  TYPESPEC, RID_UNSIGNED,},
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register char *s = wordlist[key].name;

          if (*s == *str && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
