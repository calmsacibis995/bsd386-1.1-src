/*
 *  These are changes needed for the VM/CMS version of dvips as of
 *  May 21, 1991.
 *  This was written by J. Hafner, E. Blanz and M. Flickner of IBM Research,
 *  Almaden Research Center.  Contact hafner@almaden.ibm.com.
 *  The changes to the rest of the code can be found by searching for
 *  "VMCMS".
 *
 *  This code can be included in dvips, subject to the same restrictions as
 *  in the copyright notice.
 */
extern char *malloc();
/*
 *  Added ascii to ebcdic conversion table for VMCMS.
 *  Only for the printable characters.  This is used by DVIPS.
*/
char ascii2ebcdic[256] = {
       0,  1,  2,  3, 53, 45, 46, 47,
      22,  5, 37, 11, 12, 64, 14, 15,
      16, 17, 18, 19, 60, 61, 50, 38,
      24, 25, 63, 39,111,111,111,111,
      64, 90,127,123, 91,108, 80,125,
      77, 93, 92, 78,107, 96, 75, 97,
     240,241,242,243,244,245,246,247,
     248,249,122, 94, 76,126,110,111,
     124,193,194,195,196,197,198,199,
     200,201,209,210,211,212,213,214,
     215,216,217,226,227,228,229,230,
     231,232,233,173,224,189, 95,109,
     121,129,130,131,132,133,134,135,
     136,137,145,146,147,148,149,150,
     151,152,153,162,163,164,165,166,
     167,168,169,192, 79,208,161,  7,
     128,129,130,131,132,133,134,135,
     136,137,138,139,140,141,142,143,
     144,145,146,147,148,149,150,151,
     152,153,154,155,156,157,158,159,
     160,161,162,163,164,165,166,167,
     168,169,170,171,172,173,174,175,
     176,177,178,179,180,181,182,183,
     184,185,186,187,188,189,190,191,
     192,193,194,195,196,197,198,199,
     200,201,202,203,204,205,206,207,
     208,209,210,211,212,213,214,215,
     216,217,218,219,220,221,222,223,
     224,225,226,227,228,229,230,231,
     232,233,234,235,236,237,238,239,
     240,241,242,243,244,245,246,247,
     248,249,250,251,252,253,254,255
};
/*
 *  Added ebcdic to ascii conversion table for VMCMS.
 *  Only for the printable characters.  This inverts the
 *  table above and is used by AFM2TFM.
*/
char ebcdic2ascii[256] = {
       0,  1,  2,  3,  4,  9,  6,127,
       8,  9, 10, 11, 12, 13, 14, 15,
      16, 17, 18, 19, 20, 21,  8, 23,
      24, 25, 26, 27, 28, 29, 30, 31,
      32, 33, 34, 35, 36, 10, 23, 27,
      40, 41, 42, 43, 44,  5,  6,  7,
      48, 49, 22, 51, 52,  4, 54, 55,
      56, 57, 58, 59, 20, 21, 62, 26,
      32, 65, 66, 67, 68, 69, 70, 71,
      72, 73, 74, 46, 60, 40, 43,124,
      38, 81, 82, 83, 84, 85, 86, 87,
      88, 89, 33, 36, 42, 41, 59, 94,
      45, 47, 98, 99,100,101,102,103,
     104,105,124, 44, 37, 95, 62, 63,
     112,113,114,115,116,117,118,119,
     120, 96, 58, 35, 64, 39, 61, 34,
     128, 97, 98, 99,100,101,102,103,
     104,105,138,123,140,141,142,143,
     144,106,107,108,109,110,111,112,
     113,114,154,125,156,157,158,159,
     160,161,115,116,117,118,119,120,
     121,122,170,171,172, 91,174,175,
     176,177,178,179,180,181,182,183,
     184,185,186,187,188, 93,190,191,
     192, 65, 66, 67, 68, 69, 70, 71,
      72, 73,202,203,204,205,206,207,
     208, 74, 75, 76, 77, 78, 79, 80,
      81, 82,218,219,220,221,222,223,
      92,225, 83, 84, 85, 86, 87, 88,
      89, 90,234,235,236,237,238,239,
      48, 49, 50, 51, 52, 53, 54, 55,
      56, 57,250,251,252,253,254,255
      };
/*
 *   This redefines fopen for VMCMS, truncating long filenames for
 *   portability of TeX source.
 */
#ifdef fopen
#undef fopen
#endif
FILE *cmsfopen(filename, mode)
char *filename, *mode ;
{
   char cmsfile[21] ;
   char *save_file;
   char *ptr;
   void exit() ;

   if (!(save_file =
       (char *)malloc(strlen(filename) + 1, sizeof(char)))) {
      fprintf (stderr, "sorry---out of memory\n");
      exit(10);
   }

   strcpy(save_file, filename);

   cmsfile[0] = '\0' ;

   ptr = strtok(save_file, ". ") ;
   strncat(cmsfile, ptr, 8) ;

   strcat(cmsfile, ".");

   if (!(ptr = strtok(NULL, ". "))) {
     fprintf(stderr, "\n<%s> is not a legitimate VM/CMS file name\n",
             filename);
     fprintf(stderr, "...we'll try to continue...\n");
     return(NULL);
   }

   strncat(cmsfile, ptr, 8);

   if (ptr = strtok(NULL, ". ")) {
      strcat(cmsfile, ".");
      strncat(cmsfile, ptr, 2);
   }

   free(save_file);

   return(fopen(cmsfile, mode));

}


/*
 * access.c for VMCMS
 */
int access (path, amode)
char *path;
int amode;
{
  return(1);
}
