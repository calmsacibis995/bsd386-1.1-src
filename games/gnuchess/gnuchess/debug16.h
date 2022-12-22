#ifdef DEBUG4
      if (debuglevel & 16)
	{
	  int j;

	  printf ("Sdepth %d alpha %d beta %d stage %d\n", Sdepth, alpha, beta, stage);
	  for (j = 1; j < 2; j++)
	    {
	      int idb;

	      for (idb = TrPnt[j]; idb < TrPnt[j + 1]; idb++)
		{
		  algbr (Tree[idb].f, Tree[idb].t, Tree[idb].flags);
		  printf ("level 16 %d-->%d %s %d %d %x\n", Sdepth, idb, mvstr[0], Tree[idb].score, Tree[idb].width, Tree[idb].flags);
		}
	    }
	}
#endif
