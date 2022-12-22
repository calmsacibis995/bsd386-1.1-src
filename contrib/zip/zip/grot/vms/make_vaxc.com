$ !
$ !     "Makefile" for VMS versions of Zip, ZipNote,
$ !     and ZipSplit (stolen from Unzip)
$ !
$ !      IMPORTANT NOTE: do not forget to set the symbols as said below
$ !                      in the Symbols section.
$ !
$ set verify    ! like "echo on", eh?
$ !
$ !------------------------------- Zip section --------------------------------
$ !
$ cc zip, zipfile, zipup, fileio, util, deflate, globals,-
	trees, bits, vms, VMSmunch
$ link zip, zipfile, zipup, fileio, util, deflate, globals,-
	trees, bits, vms, VMSmunch, sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ !
$ !-------------------------- Zip utilities section ---------------------------
$ !
$ cc /def=UTIL /obj=zipfile_.obj zipfile.c
$ cc /def=UTIL /obj=zipup_.obj zipup.c
$ cc /def=UTIL /obj=fileio_.obj fileio.c
$ cc /def=UTIL /obj=util_.obj util.c
$ cc zipnote, zipsplit
$ link zipnote, zipfile_, zipup_, fileio_, util_, globals, VMSmunch,-
	sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ link zipsplit, zipfile_, zipup_, fileio_, util_, globals, VMSmunch,-
	sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ !
$ !----------------------------- Symbols section ------------------------------
$ !
$ ! Set up symbols for the various executables.  Edit the example below,
$ ! changing "disk:[directory]" as appropriate.
$ !
$ zip		== "$disk:[directory]zip.exe"
$ zipnote	== "$disk:[directory]zipnote.exe"
$ zipsplit	== "$disk:[directory]zipsplit.exe"
$ !
$ set noverify
