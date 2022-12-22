#!/usr/bin/perl

$numentries = $ARGV[0];

open (TFILE, ">master.passwd") || die "Couldn't open master.passwd";

print TFILE <<EOF;
root:fMvyVcAoW8iXk:0:0::0:0:System Administrator:/root:/bin/csh
daemon:*:1:1::0:0:System Daemon:/root:
sys:*:2:2::0:0:Operating System:/tmp:/bin/csh
bin:*:3:7::0:0:BSDI Software:/usr/bsdi:/bin/csh
operator:*:5:5::0:0:System Operator:/usr/opr:/bin/csh
uucp:*:6:6::0:0:UNIX-to-UNIX Copy:/var/spool/uucppublic:/usr/libexec/uucico
games:*:7:13::0:0:Games Pseudo-user:/usr/games:
nobody:*:32767:32766::0:0:Unprivileged user:/nonexistent:/dev/null
demo:*:10:13::0:0:Demo User:/usr/demo:/bin/csh
polk:0t7sowvHNCGRQ:12608:10::0:0:Jeff Polk,Colo. Spgs.,719-260-8114,:/home/hilltop/polk:/bin/bash
EOF

for ($i=0; $i<$numentries; $i++) {
	printf TFILE "user%d:*:%d:10::0:0:Extra User %d,,,:/home/user%d:/bin/bash\n",
		$i, $i, $i, $i;
}
close (TFILE);
