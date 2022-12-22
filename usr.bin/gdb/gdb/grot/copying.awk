BEGIN	{
	  FS="\"";
	  print "/* ==> Do not modify this file!!  It is created automatically";
	  print "   by copying.awk.  Modify copying.awk instead.  <== */";
	  print ""
	  print "#include <stdio.h>"
	  print "#include \"defs.h\""
	  print "#include \"command.h\""
	  print "#include \"gdbcmd.h\""
	  print ""
	  print "static void"
	  print "copying_info PARAMS ((char *, int));"
	  print ""
	  print "static void"
	  print "warranty_info PARAMS ((char *, int));"
	  print ""
	  print "extern int immediate_quit;";
	  print "static void";
	  print "copying_info (ignore, from_tty)";
	  print "     char *ignore;";
	  print "     int from_tty;";
	  print "{";
	  print "  immediate_quit++;";
	}
NR == 1,/^[ 	]*NO WARRANTY[ 	]*$/	{
	  if ($0 ~ //)
	    {
	      printf "  printf_filtered (\"\\n\");\n";
	    }
	  else if ($0 !~ /^[ 	]*NO WARRANTY[ 	]*$/) 
	    {
	      printf "  printf_filtered (\"";
	      for (i = 1; i < NF; i++)
		printf "%s\\\"", $i;
	      printf "%s\\n\");\n", $NF;
	    }
	}
/^[	 ]*NO WARRANTY[ 	]*$/	{
	  print "  immediate_quit--;";
	  print "}";
	  print "";
	  print "static void";
	  print "warranty_info (ignore, from_tty)";
	  print "     char *ignore;";
	  print "     int from_tty;";
	  print "{";
	  print "  immediate_quit++;";
	}
/^[ 	]*NO WARRANTY[ 	]*$/, /^[ 	]*END OF TERMS AND CONDITIONS[ 	]*$/{  
	  if (! ($0 ~ /^[ 	]*END OF TERMS AND CONDITIONS[ 	]*$/)) 
	    {
	      printf "  printf_filtered (\"";
	      for (i = 1; i < NF; i++)
		printf "%s\\\"", $i;
	      printf "%s\\n\");\n", $NF;
	    }
	}
END	{
	  print "  immediate_quit--;";
	  print "}";
	  print "";
	  print "void"
	  print "_initialize_copying ()";
	  print "{";
	  print "  add_info (\"copying\", copying_info,";
	  print "	    \"Conditions for redistributing copies of GDB.\");";
	  print "  add_info (\"warranty\", warranty_info,";
	  print "	    \"Various kinds of warranty you do not have.\");";
	  print "  add_cmd (\"copying\", no_class, copying_info,";
	  print "	   \"Conditions for redistributing copies of GDB.\",";
	  print "	   &showlist);";
	  print "  add_cmd (\"warranty\", no_class, warranty_info,";
	  print "	   \"Various kinds of warranty you do not have.\",";
	  print "	   &showlist);";
	  print "}";
	}
