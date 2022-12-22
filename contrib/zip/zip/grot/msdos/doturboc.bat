: This file is a complement to zip.prj for Turbo C 2.0 users.
: Change the compilation flags if you wish (add SMALL_MEM or MEDIUM_MEM
: to reduce the memory requirements), and press F9...
: WARNING: you must use the compact or large model in this version.

tasm -ml -DDYN_ALLOC -DSS_NEQ_DS match;

: To compile zipsplit and zipnote, delete all .obj files, add the compilation
: flag UTIL, and use zipsplit.prj or zipnote.prj to recompile everything.
