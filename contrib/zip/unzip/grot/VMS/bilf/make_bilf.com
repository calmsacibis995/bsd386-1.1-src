$ set verify	! like "echo on", eh?
$ cc bilf
$ link bilf,sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ ! change the following symbol to "$diskname:[directory]bilf.exe" as approp.
$ bilf == "$bilf.exe"
$ set noverify
