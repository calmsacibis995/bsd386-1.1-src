$ !
$ !     "Makefile" for VMS versions of Zip, ZipCloak, ZipNote,
$ !      and ZipSplit (stolen from Unzip)
$ !
$ !      IMPORTANT NOTE: do not forget to set the symbols as said below
$ !                      in the Symbols section.
$ !
$ set verify    ! like "echo on", eh?
$ !
$ !------------------------------- Zip section --------------------------------
$ !
$ ! For crypt version, use version of make_gcc.com which in the zcrypt
$ ! distribution.
$ gcc zip
$ gcc zipfile
$ gcc zipup
$ gcc fileio
$ gcc util
$ gcc deflate
$ gcc globals
$ gcc trees
$ gcc bits
$ gcc vms
$ gcc VMSmunch
$ link zip,zipfile,zipup,fileio,util,deflate,globals,trees,bits, -
   vms,VMSmunch, gnu_cc:[000000]gcclib.olb/lib, sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ !
$ !-------------------------- Zip utilities section ---------------------------
$ !
$ gcc /def=UTIL zipnote
$ gcc /def=UTIL zipsplit
$ gcc /def=UTIL /obj=zipfile_.obj zipfile
$ gcc /def=UTIL /obj=zipup_.obj zipup
$ gcc /def=UTIL /obj=fileio_.obj fileio
$ gcc /def=UTIL /obj=util_.obj util
$ link zipnote, zipfile_, zipup_, fileio_, util_, globals, VMSmunch, -
   sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ link zipsplit, zipfile_, zipup_, fileio_, util_, globals, VMSmunch, -
   gnu_cc:[000000]gcclib.olb/lib, sys$input:/opt
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
