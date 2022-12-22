/* -*-C-*- usage.h */
/*-->usage*/
/**********************************************************************/
/******************************* usage ********************************/
/**********************************************************************/

void
usage(fp)		/* print usage message to fp and return */
FILE *fp;
{
    (void)fprintf(fp,"[TeX82 DVI Translator Version %s]",VERSION_NO);
    NEWLINE(fp);
    (void)fprintf(fp,"[%s]",DEVICE_ID);
    NEWLINE(fp);

    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-x#{units}} {-y#{units}} dvifile(s)",
g_progname);

#if    APPLEIMAGEWRITER
    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-r} {-x#{units}} {-y#{units}} \
dvifile(s)",
g_progname);
#endif

#if    EPSON
    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-r} {-t} {-x#{units}} {-y#{units}} \
{-z} dvifile(s)",
g_progname);
#endif /* EPSON */

#if    GOLDENDAWNGL100
    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-r} {-x#{units}} {-y#{units}} \
dvifile(s)",
g_progname);
#endif

#if    HPLASERJET
    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-j} {-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-r#} {-x#{units}} {-y#{units}} \
{-z} dvifile(s)",
g_progname);
#endif /* HPLASERJET */

#if    POSTSCRIPT
    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-r} {-v} {-x#{units}} {-y#{units}} \
{-z} dvifile(s)",
g_progname);
#endif /* POSTSCRIPT */

#if    TOSHIBAP1351
    (void)sprintf(message,
        "Usage: %s {-a} {-b} {-c#} {-d#} {-eENVNAME=value} {-ffontsubfile} \
{-l} {-m#} {-o#:#:#} {-o#:#} {-o#} {-p} {-r} {-x#{units}} {-y#{units}} \
dvifile(s)",
g_progname);
#endif

    (void)fprintf(fp,message);
    NEWLINE(fp);


    (void)fprintf(fp,
	"For documentation on this program, try the operating command(s):");
    NEWLINE(fp);

    (void)fprintf(fp,helpcmd);
    NEWLINE(fp);
    NEWLINE(fp);
}
