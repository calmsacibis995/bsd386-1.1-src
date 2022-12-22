$!
This command file generates all the .EXE and .PRO files needed for the
$! installation of dvips on a VAX/VMS system from the distribution.
$! Simply amend the path definitions as required, and then type @make 
$! in the VMS sub-directory.
$!
$!  Rob Hutchings   5-MAY-1992
$!  Oxford University Computing Service
$!  oxtex@vax.oxford.ac.uk
$!
$ on error then goto fail
$ on severe_error then goto fail
$
$ set verify
$ cc /define=(VMS,DEBUG,-
	"TFMPATH=""TEX$FONTS:""",-       
	"PKPATH=""TEX_PK:""",-
	"VFPATH=""TEX_VF:""",-
	"FIGPATH=""TEX_INPUTS:""",-
	"CONFIGPATH=""TEX$POSTSCRIPT:""",-
	"HEADERPATH=""TEX$POSTSCRIPT:,SYS$LOGIN:,""") [-]dvips
$ cc /define=(VMS,DEBUG,TPIC) vaxvms, [-]afm2tfm, squeeze, -
	dopage, dosection, dospecial, download, drawps, dpicheck, dviinput, -
	finclude, fontdef, header, loadfont, makefont, output, pprescan, -
	prescan, repack, resident, scalewidth, scanpage, search, skippage, -
	tfmload, unpack, virtualfont, papersiz, color, bbox
$ link dvips, vaxvms, vaxcrtl.opt/opt, -
	dopage, dosection, dospecial, download, drawps, dpicheck, dviinput, -
	finclude, fontdef, header, loadfont, makefont, output, pprescan, -
	prescan, repack, resident, scalewidth, scanpage, search, skippage, -
	tfmload, unpack, virtualfont, papersiz, color, bbox
$ link squeeze, vaxvms, vaxcrtl.opt/opt
$ link afm2tfm, vaxvms, vaxcrtl.opt/opt
$ squeeze := $'f$environment("default")'squeeze
$ squeeze [-]color.lpro color.pro
$ squeeze [-]crop.lpro crop.pro
$ squeeze [-]finclude.lpro finclude.pro
$ squeeze [-]special.lpro special.pro
$ squeeze [-]tex.lpro tex.pro
$ squeeze [-]texps.lpro texps.pro
$ set noverify
$!----------------------------------------------------------------------
$!
$!                            TEXC_SCRIPT.COM
$!
$!  Convert TEX.LPRO to TEXC.LPRO
$!  This command file replaces the TexC.script file, intended for Unix OS,
$!  with its TPU equivalent.
$!
$!  Tony McGrath    5-OCT-1990
$!  Dept. of Physics, Monash University, Victoria, Australia 3168
$!
$!  Slightly simplified version by Rob Hutchings
$!
$!----------------------------------------------------------------------
$!
$ Edit/TPU /NoSection /NoDisplay /Command=SYS$Input 
!-----------------------------------------------------------------------
! Initialize the main buffer.
!-----------------------------------------------------------------------
b:=Create_Buffer("","[-]tex.lpro");
Set (Output_File,b,"texc.lpro");
Position (Beginning_of(b));
!-----------------------------------------------------------------------
! Won't bother with CREATE_ARRAY, try to keep the TPU as basic as possible
! so it works on older versions of VMS.
!-----------------------------------------------------------------------
string_1 := "% begin code for uncompressed fonts only";
string_2 := "% end code for uncompressed fonts only";
string_3 := "% end of code for unpacking compressed fonts";
!-----------------------------------------------------------------------
! Search for the first of the 3 special strings.
! Exit if we can't find it.
!-----------------------------------------------------------------------
a_range  := search( string_1, forward);
if( a_range = 0)
then
  message( "TEXC-F-NoString, Couldn't locate first string, Aborting");
  Quit;
endif;
!-----------------------------------------------------------------------
! Go to the start of the first string.
!-----------------------------------------------------------------------
position( beginning_of( a_range));
!-----------------------------------------------------------------------
! Search for the second of the 3 special strings.
! Exit if we can't find it.
!-----------------------------------------------------------------------
a_range  := search( string_2, forward);
if( a_range = 0)
then
  message( "TEXC-F-NoString, Couldn't locate second string, Aborting");
  Quit;
endif;
!-----------------------------------------------------------------------
! Then start deleting lines until the second special string is found.
!-----------------------------------------------------------------------
loop
  a_line := erase_line;
  exitif a_line = string_2;
endloop;
!-----------------------------------------------------------------------
! Search for the third of the 3 special strings.
! Exit if we can't find it.
!-----------------------------------------------------------------------
a_range  := search( string_3, forward);
if( a_range = 0)
then
  message( "TEXC-F-NoString, Couldn't locate third string, Aborting");
  Quit;
endif;
!-----------------------------------------------------------------------
! Again start looping, deleting the first 2 characters from each line
! until the 3rd special string is found, making sure that the first two
! characters are "% "
!-----------------------------------------------------------------------
loop
  two_chars := erase_character(2);
  if two_chars <> "% "
  then
    message( "TEXC-F-NoComment, First 2 chars not correct, Aborting");
    Quit;
  endif;
  exitif current_line = string_3;
  move_vertical(1);
endloop;
Exit;
$ squeeze texc.lpro texc.pro
$ delete texc.lpro;,*.obj;
$ exit
$ fail:
$   set noverify
$   write sys$output "Uh Oh. Some sort of error happened."
$ exit 
