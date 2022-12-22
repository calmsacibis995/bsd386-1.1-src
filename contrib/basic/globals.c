#include "structs.h"

char    CurFile[50];

char    Opts[128];
int    TimeToQuit;
int    TerminalInput;

struct tree_node *NextData;
long    DataPC;
int     DataCount;

char   *IPtr;			/* input pointer */
YYSTYPE yylval;

long    PC;
long    OldPC;
struct line_descr *PCP;

int     Break;
int     Stop;
int     SingleStepping;
int     FromTerminal;
int     OutputCol;

long StmtCount;

long    GosubStack[GOSUB_DEPTH], *GP;

struct value Stack[EXPR_DEPTH], *SP;

struct for_context        ForStack[FOR_DEPTH], *FP;

char    InputLine[LINE_LEN];
int    InputPrinted, SaveStrings;
char   *IPtr;			/* input pointer */
int    LookForCmd;

struct line_descr *FirstLine;
struct line_descr *LastLine;
struct line_descr *FastTab;
unsigned FastCount, FastSize;
int     ChangesMade;

struct line_descr *FindLine ();

char    StringSpace[STRING_TEMP_SIZE], *StringTempPtr;
double *DoubleSpace;
int    *IntSpace;

jmp_buf	ExecJmpBuf;
long MasterSymbolVersion = 1;
long screenwidth;
