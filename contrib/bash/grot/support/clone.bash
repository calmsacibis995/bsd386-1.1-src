#!/usr/gnu/bin/bash
#

# Clone the current directory, making links into ../----
#
# The cloned directory is called whatever this directory is called.

function clone {
   local target
   if [ "$target" = "" ]; then
      target=$(basename $(pwd))
   fi
   clone_internal "" $target 0 .[a-z0-9A-Z_-]* *
}

function clone_internal {
   local prefix=$1 target=$2 level=$3
   local dot_prefix counter

   shift 3

   mkdir $target
   cd $target
   level=$[level + 1]
   counter=$level
   while [ $counter != "0" ]; do
      dot_prefix=${dot_prefix}../
      counter=$[counter - 1]
   done
   dot_prefix=${dot_prefix}${prefix}

   for file; do
      if [ ! -d $dot_prefix$file ]; then
	 ln -s $dot_prefix$file .
      else
	 clone_internal ${prefix}${file}/ $file $level \
	 $(cd $dot_prefix$file; echo *)
      fi
   done
   cd ..
}

	 