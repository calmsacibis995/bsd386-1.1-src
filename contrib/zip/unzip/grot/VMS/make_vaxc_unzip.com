$ !
$ !		"Makefile" for VMS versions of UnZip and ZipInfo
$ !				(version:  VAX C)
$ !
$ ! Find out current disk and directory
$ !
$ my_name = f$env("procedure")
$ here = f$parse(my_name,,,"device") + f$parse(my_name,,,"directory")
$ set verify	! like "echo on", eh?
$ !
$ ! Do UnZip:
$ !   (for decryption version, add /def=CRYPT to compile line, and add
$ !   crypt to both compile and link lines)
$ !
$ cc unzip, envargs, explode, extract, file_io, inflate, mapname,-
	match, misc, unreduce, unshrink, vms
$ link unzip, envargs, explode, extract, file_io, inflate, mapname,-
	match, misc, unreduce, unshrink, vms, sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
$ !
$ ! Do ZipInfo:
$ !
$ cc zipinfo
$ cc /def=(ZIPINFO) /obj=misc_.obj  misc.c
$ cc /def=(ZIPINFO) /obj=vms_.obj   vms.c
$ link zipinfo, envargs, match, misc_, vms_, sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
$ !
$ ! Next line:  put a similar line (full pathname for unzip.exe and zipinfo.exe)
$ ! in login.com.  Remember to include the leading "$" before disk name.
$ !
$ unzip == "$''here'unzip.exe"		! set up symbol to use unzip
$ zipinfo == "$''here'zipinfo.exe"	! set up symbol to use zipinfo
$ !
$ set noverify
