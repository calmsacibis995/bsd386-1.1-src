$ !
$ !		"Makefile" for VMS versions of UnZip and ZipInfo
$ !				(version:  GNU C)
$ !
$ ! Find out current disk and directory
$ !
$ my_name = f$env("procedure")
$ here = f$parse(my_name,,,"device") + f$parse(my_name,,,"directory")
$ set verify	! like "echo on", eh?
$ !
$ ! Do UnZip:
$ !   (for decryption version, add /def=CRYPT to each of following lines,
$ !   uncomment crypt compile line, and add crypt to link line)
$ !
$ gcc unzip
$ gcc envargs
$ gcc explode
$ gcc extract
$ gcc file_io
$ gcc inflate
$ gcc mapname
$ gcc match
$ gcc misc
$ gcc unreduce
$ gcc unshrink
$ gcc vms
$! gcc crypt
$ link unzip, envargs, explode, extract, file_io, inflate, mapname,-
	match, misc, unreduce, unshrink, vms, gnu_cc:[000000]gcclib.olb/lib,-
	sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
$ !
$ ! Do ZipInfo:
$ !
$ gcc zipinfo
$ gcc /def=(ZIPINFO) /object=misc_.obj misc
$ gcc /def=(ZIPINFO) /object=vms_.obj  vms
$ link zipinfo, envargs, match, misc_, vms_, gnu_cc:[000000]gcclib.olb/lib,-
	sys$input:/opt
 sys$share:vaxcrtl.exe/shareable
$ !
$ ! Next line:  put a similar line (full pathname for unzip.exe and zipinfo.exe)
$ ! in login.com.  Remember to include the leading "$" before disk name.
$ !
$ unzip == "$''here'unzip.exe"		! set up symbol to use unzip
$ zipinfo == "$''here'zipinfo.exe"	! set up symbol to use zipinfo
$ !
$ set noverify
