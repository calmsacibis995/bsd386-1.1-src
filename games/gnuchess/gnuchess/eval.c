/*
 * eval.c - C source for GNU CHESS
 *
 * Copyright (c) 1988,1989,1990 John Stanback
 * Copyright (c) 1992 Free Software Foundation
 *
 * This file is part of GNU CHESS.
 *
 * GNU Chess is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU Chess is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Chess; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "gnuchess.h"
#include "ataks.h"
int EADD = 0;
int EGET = 0;
int PUTVAR = false;
#ifdef CACHE
struct etable etab[2][ETABLE];
#endif

short int sscore[2];
/* Backward pawn bonus indexed by # of attackers on the square */
static const short BACKWARD[16] =
{-6, -10, -15, -21, -28, -28, -28, -28, -28, -28, -28, -28, -28, -28, -28, -28};

/* Bishop mobility bonus indexed by # reachable squares */
static const short BMBLTY[14] =
{-2, 0, 2, 4, 6, 8, 10, 12, 13, 14, 15, 16, 16, 16};

/* Rook mobility bonus indexed by # reachable squares */
static const short RMBLTY[15] =
{0, 2, 4, 6, 8, 10, 11, 12, 13, 14, 14, 14, 14, 14, 14};

/* Positional values for a dying king */
static const short DyingKing[64] =
{0, 8, 16, 24, 24, 16, 8, 0,
 8, 32, 40, 48, 48, 40, 32, 8,
 16, 40, 56, 64, 64, 56, 40, 16,
 24, 48, 64, 72, 72, 64, 48, 24,
 24, 48, 64, 72, 72, 64, 48, 24,
 16, 40, 56, 64, 64, 56, 40, 16,
 8, 32, 40, 48, 48, 40, 32, 8,
 0, 8, 16, 24, 24, 16, 8, 0};

/* Isoloted pawn penalty by rank */
static const short ISOLANI[8] =
{-12, -16, -20, -24, -24, -20, -16, -12};

/* table for King Bishop Knight endings */
static const short KBNK[64] =
{99, 90, 80, 70, 60, 50, 40, 40,
 90, 80, 60, 50, 40, 30, 20, 40,
 80, 60, 40, 30, 20, 10, 30, 50,
 70, 50, 30, 10, 0, 20, 40, 60,
 60, 40, 20, 0, 10, 30, 50, 70,
 50, 30, 10, 20, 30, 40, 60, 80,
 40, 20, 30, 40, 50, 60, 80, 90,
 40, 40, 50, 60, 70, 80, 90, 99};

/* penalty for threats to king, indexed by number of such threats */
static const short KTHRT[36] =
{0, -8, -20, -36, -52, -68, -80, -80, -80, -80, -80, -80,
 -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80,
 -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80};

/* King positional bonus inopening stage */
static const short KingOpening[64] =
{0, 0, -4, -10, -10, -4, 0, 0,
 -4, -4, -8, -12, -12, -8, -4, -4,
 -12, -16, -20, -20, -20, -20, -16, -12,
 -16, -20, -24, -24, -24, -24, -20, -16,
 -16, -20, -24, -24, -24, -24, -20, -16,
 -12, -16, -20, -20, -20, -20, -16, -12,
 -4, -4, -8, -12, -12, -8, -4, -4,
 0, 0, -4, -10, -10, -4, 0, 0};

/* King positional bonus in end stage */
static const short KingEnding[64] =
{0, 6, 12, 18, 18, 12, 6, 0,
 6, 12, 18, 24, 24, 18, 12, 6,
 12, 18, 24, 30, 30, 24, 18, 12,
 18, 24, 30, 36, 36, 30, 24, 18,
 18, 24, 30, 36, 36, 30, 24, 18,
 12, 18, 24, 30, 30, 24, 18, 12,
 6, 12, 18, 24, 24, 18, 12, 6,
 0, 6, 12, 18, 18, 12, 6, 0};

/* Passed pawn positional bonus */
static const short PassedPawn0[8] =
{0, 60, 80, 120, 200, 360, 600, 800};
static const short PassedPawn1[8] =
{0, 30, 40, 60, 100, 180, 300, 800};
static const short PassedPawn2[8] =
{0, 15, 25, 35, 50, 90, 140, 800};
static const short PassedPawn3[8] =
{0, 5, 10, 15, 20, 30, 140, 800};

/* Knight positional bonus */
static const short pknight[64] =
{0, 4, 8, 10, 10, 8, 4, 0,
 4, 8, 16, 20, 20, 16, 8, 4,
 8, 16, 24, 28, 28, 24, 16, 8,
 10, 20, 28, 32, 32, 28, 20, 10,
 10, 20, 28, 32, 32, 28, 20, 10,
 8, 16, 24, 28, 28, 24, 16, 8,
 4, 8, 16, 20, 20, 16, 8, 4,
 0, 4, 8, 10, 10, 8, 4, 0};

/* Bishop positional bonus */
static const short pbishop[64] =
{14, 14, 14, 14, 14, 14, 14, 14,
 14, 22, 18, 18, 18, 18, 22, 14,
 14, 18, 22, 22, 22, 22, 18, 14,
 14, 18, 22, 22, 22, 22, 18, 14,
 14, 18, 22, 22, 22, 22, 18, 14,
 14, 18, 22, 22, 22, 22, 18, 14,
 14, 22, 18, 18, 18, 18, 22, 14,
 14, 14, 14, 14, 14, 14, 14, 14};

/* Pawn positional bonus */
static const short PawnAdvance[64] =
{0, 0, 0, 0, 0, 0, 0, 0,
 4, 4, 4, 0, 0, 4, 4, 4,
 6, 8, 2, 10, 10, 2, 8, 6,
 6, 8, 12, 16, 16, 12, 8, 6,
 8, 12, 16, 24, 24, 16, 12, 8,
 12, 16, 24, 32, 32, 24, 16, 12,
 12, 16, 24, 32, 32, 24, 16, 12,
 0, 0, 0, 0, 0, 0, 0, 0};

short Mwpawn[64], Mbpawn[64], Mknight[2][64], Mbishop[2][64];
static short Mking[2][64], Kfield[2][64];
static short c1, c2, *atk1, *atk2, *PC1, *PC2, atak[2][64];
short emtl[2];
static short PawnBonus, BishopBonus, RookBonus;
static short KNIGHTPOST, KNIGHTSTRONG, BISHOPSTRONG, KATAK;
static short PEDRNK2B, PWEAKH, PADVNCM, PADVNCI, PAWNSHIELD, PDOUBLED, PBLOK;
static short RHOPN, RHOPNX, KHOPN, KHOPNX, KSFTY;
static short ATAKD, HUNGP, HUNGX, KCASTLD, KMOVD, XRAY, PINVAL;
short pscore[2];
short tmtl;

#ifdef CACHE
inline void
PutInEETable (short int side,int score)

/*
 * Store the current eval position in the transposition table.
 */

{
    register struct etable *ptbl;
    ptbl = &etab[side][hashkey % (ETABLE)];
    ptbl->ehashbd = hashbd;
    ptbl->escore[white] = pscore[white];
    ptbl->escore[black] = pscore[black];
    ptbl->hung[white] = hung[white];
    ptbl->hung[black] = hung[black];
    ptbl->score = score;
    bcopy (svalue, &(ptbl->sscore), sizeof (svalue));
#ifndef BAREBONES 
    EADD++;
#endif
    return;
}

inline int
CheckEETable (short int side)

/* Get an evaluation from the transposition table */
{
    register struct etable *ptbl;
    ptbl = &etab[side][hashkey % (ETABLE)];
    if (hashbd == ptbl->ehashbd) return true;
    return false;
}

inline int
ProbeEETable (short int side, short int *score)

/* Get an evaluation from the transposition table */
{
    register struct etable *ptbl;
    ptbl = &etab[side][hashkey % (ETABLE)];
    if (hashbd == ptbl->ehashbd)
      {
	  pscore[white] = ptbl->escore[white];
	  pscore[black] = ptbl->escore[black];
	  bcopy (&(ptbl->sscore),svalue, sizeof (svalue));
	  *score = ptbl->score;
          hung[white] = ptbl->hung[white];
          hung[black] = ptbl->hung[black];
#ifndef BAREBONES
	  EGET++;
#endif
	  return true;
      }
    return false;

}

#endif
/* ............    POSITIONAL EVALUATION ROUTINES    ............ */

/*
 * Inputs are:
 * pmtl[side] - value of pawns
 * mtl[side]  - value of all material
 * emtl[side] - vaule of all material - value of pawns - value of king
 * hung[side] - count of hung pieces
 * Tscore[ply] - search tree score for ply
 * ply
 * Pscore[ply] - positional score for ply ply
 * INCscore    - bonus score or penalty for certain positions
 * slk - single lone king flag
 * Sdepth - search goal depth
 * xwndw - evaluation window about alpha/beta
 * EWNDW - second evaluation window about alpha/beta
 * ChkFlag[ply]- checking piece at level ply or 0 if no check
 * PC1[column] - # of my pawns in this column
 * PC2[column] - # of opponents pawns in column
 * PieceCnt[side] - just what it says
 */
inline
int
ScoreKPK (short int side,
	  short int winner,
	  short int loser,
	  short int king1,
	  register short int king2,
	  register short int sq)

/*
 * Score King and Pawns versus King endings.
 */

{
    register short s, r;

    s = ((PieceCnt[winner] == 1) ? 50 : 120);
    if (winner == white)
      {
	  r = row (sq) - ((side == loser) ? 1 : 0);
	  if (row (king2) >= r && distance (sq, king2) < 8 - r)
	      s += 10 * row (sq);
	  else
	      s = 500 + 50 * row (sq);
	  if (row (sq) < 6)
	      sq += 16;
	  else if (row (sq) == 6)
	      sq += 8;
      }
    else
      {
	  r = row (sq) + ((side == loser) ? 1 : 0);
	  if (row (king2) <= r && distance (sq, king2) < r + 1)
	      s += 10 * (7 - row (sq));
	  else
	      s = 500 + 50 * (7 - row (sq));
	  if (row (sq) > 1)
	      sq -= 16;
	  else if (row (sq) == 1)
	      sq -= 8;
      }
    s += 8 * (taxicab (king2, sq) - taxicab (king1, sq));
    return (s);
}

inline short dist_ (short f1, short r1, short f2, short r2)
{
  return distdata [ f1-9+8*r1 ][ f2-9+8*r2 ];
}

short 
kpkwv_ (short pf, short pr, short wf, short wr, short bf, short br)
{

  /*
   *  Don Beal's routine, which was originally in Fortran.  See AICC 2
   */


  const short drawn=0, win=1;
  
  short wbdd, mbpf, nbpf, blpu;
  short brpu, mwpf, wlpu, wrpu, blpuu, brpuu, wlpuu, wrpuu, md,
    bq, blpuuu, brpuuu, bsd, tbf, sgf, bpp, sdr, sgr, wsd, wsg, ppr, wpp;


  ppr = pr;
  if (pr == 2) { ppr = 3; }
  if (pf != 1) { goto L2; }
  if (bf != 3) { goto L1; }
  if (pr == 7 && wf == 1 && wr == 8 && br > 6) return drawn;
  if (pr == 6 && wf < 4 && wr == 6 && br == 8) return win;
L1:
  if (bf == 1 && br > pr) return drawn;
  if (pr == 7 && bf > 2) return win;
  if (bf <= 3 && br - ppr > 1) return drawn;
  if (wf == 1 && bf == 3 && wr - pr == 1 && br - pr == 1) return drawn;
L2:
  bq = dist_ (bf, br, pf, 8);
  if (bq > 8 - ppr) return win;
  mbpf = bf - pf;
  if (mbpf < 0) { mbpf = -mbpf; }
  bpp = dist_ (bf, br, pf, ppr);
  wpp = dist_ (wf, wr, pf, ppr);
  if (bpp - wpp < -1 && br - pr != mbpf) return drawn;
  if (pf == 1 && pr <= 3 && wf <= 2 && wr == 8 && bf == 4 && br >= 7) return win;
  if (pf != 2 || pr != 6 || bf != 1 || br != 8) { goto L3; }
  if (wf <= 3 && wr == 6) return drawn;
  if (wf == 4 && wr == 8) return drawn;
L3:
  if (pr != 7) { goto L4; }
  if (wr < 8 && wpp == 2 && bq == 0) return win;
  if (wr == 6 && wf == pf && bq == 0) return win;
  if (wr >= 6 && wpp <= 2 && bq != 0) return win;
L4:
  blpuu = dist_ (bf, br, pf-1, pr+2);
  wbdd = dist_ (wf, wr, bf, br-2);
  brpuu = dist_ (bf, br, pf+1, pr+2);
  if (pr != 6) { goto L6; }
  if (dist_ (bf, br, pf+1, pr) > 1 && brpuu > dist_ (wf, wr, pf+1, pr)) return win;
  if (pf == 1) { goto L5; }
  if (blpuu > dist_ (wf, wr, pf-1, pr)) return win;
  if (br == 8 && mbpf == 1 && wbdd == 1) return win;
  if (br > 6 && mbpf == 2 && dist_ (wf, wr, bf, 5) <= 1) return win;
  goto L6;
L5:
  if (wf == 1 && wr == 8 && bf == 2 && br == 6) return drawn;
L6:
  mwpf = wf - pf;
  if (mwpf < 0) { mwpf = -mwpf; }
  if (pr >= 5 && mwpf == 2 && wr == pr && bf == wf && br - pr == 2) return win;
  brpu = dist_ (bf, br, pf+1, pr+1);
  wrpu = dist_ (wf, wr, pf+1, pr+1);
  blpu = dist_ (bf, br, pf-1, pr+1);
  wlpu = dist_ (wf, wr, pf-1, pr+1);
  if (pf == 1 || pr != 5) { goto L7; }
  if (mwpf <= 1 && wr - pr == 1) return win;
  if (wrpu == 1 && brpu > 1) return win;
  if (wr >= 4 && bf == wf && br - pr >= 2 && mbpf == 3) return win;
  if (wlpu == 1 && blpu > 1) return win;
L7:
  if (pr == 2 && br == 3 && mbpf > 1 && dist_ (wf, wr, bf, br+2) <= 1) return win;
  if (wr - pr == 2 && br == pr && mbpf == 1 && mwpf > 1 && (wf - pf) *
      (bf - pf) > 0) return drawn;
  if (pf == 1 && wf == 1 && wr == br && bf > 3) return win;
  sgf = pf - 1;
  if (wf >= pf) { sgf = pf + 1; }
  sgr = wr - (mwpf - 1);
  if (mwpf == 0 && wr > br) { sgr = wr - 1; }
  wsg = dist_ (wf, wr, sgf, sgr);
  if (wr - pr - mwpf > 0 && wr - br >= -1 && bpp - (wsg + (sgr - ppr))
      >= -1 && dist_ (bf, br, sgf, sgr) > wsg) return win;
  md = mbpf - mwpf;
  if (pf != 1 || bf <= 3) { goto L8; }
  sdr = br + (bf - 3);
  if (sdr > 8) { sdr = 8; }
  if (wr > br + 1) { sdr = br; }
  if (sdr <= ppr) { goto L8; }
  wsd = dist_ (wf, wr, 3, sdr);
  bsd = dist_ (bf, br, 3, sdr);
  if (bsd - wsd < -1) return drawn;
  if (bsd <= wsd && md <= 0) return drawn;
L8:
  brpuuu = dist_ (bf, br, pf+1, pr+3);
  if (brpu > wrpu && brpuuu > wrpu && pr - wr != pf - wf) return win;
  if (brpuuu == 0 && wrpu == 1) return win;
  blpuuu = dist_ (bf, br, pf-1, pr+3);
  if (pf == 1) { goto L9; }
  if (blpu > wlpu && blpuuu > wlpu && pr - wr != wf - pf) return win;
  if (blpuuu == 0 && wlpu == 1) return win;
L9:
  wrpuu = dist_ (wf, wr, pf+1, pr+2);
  if (brpuu > wrpuu) return win;
  wlpuu = dist_ (wf, wr, pf-1, pr+2);
  if (pf > 1 && blpuu > wlpuu) return win;
  if (br != pr) { goto L10; }
  if (mwpf <= 2 && wr - pr == -1 && mbpf != 2) return win;
  if (dist_ (wf, wr, bf-1, br+2) <= 1 && bf - pf > 1) return win;
  if (dist_ (wf, wr, bf+1, br+2) <= 1 && bf - pf < -1) return win;
L10:
  if (pf == 1) { goto L11; }
  if (br == pr && mbpf > 1 && dist_ (wf, wr, pf, pr-1) <= 1) return win;
  if (br - pr >= 3 && wbdd == 1) return win;
  if (wr - pr >= 2 && wr < br && md >= 0) return win;
  if (mwpf <= 2 && wr - pr >= 3 && bf != pf && wr - br <= 1) return win;
  if (wr >= pr && br - pr >= 5 && mbpf >= 3 && md >= -1 && ppr == 3) return win;
  if (md >= -1 && pr == 2 && br == 8) return win;
L11:
  tbf = bf - 1;
  if (pf > bf) { tbf = bf + 1; }
  if (mbpf > 1 && br == ppr && dist_ (wf, wr, tbf, wr+2) <= 1) return win;
  if (br == pr && bf - pf == -2 && dist_ (wf, wr, pf+2, pr-1) <= 1) return win;
  if (pf > 2 && br == pr && bf - pf == 2 && dist_ (wf, wr, pf-2, pr-1) <= 1) return win;

  return drawn;
}				/* kpkwv_ */

short 
kpkbv_ (short pf, short pr, short wf, short wr, short bf, short br)
{
  /* Initialized data */

  static short incf[8] =
  {0, 1, 1, 1, 0, -1, -1, -1};
  static short incr[8] =
  {1, 1, 0, -1, -1, -1, 0, 1};

  /* System generated locals */
  short ret_val;

  /* Local variables */
  static short i;
  static short nm, nbf, nbr;

  ret_val = 0;
  nm = 0;
  for (i = 1; i <= 8; ++i)
    {
      nbf = bf + incf[i - 1];
      if (nbf < 1 || nbf > 8) { goto L1; }
      nbr = br + incr[i - 1];
      if (nbr < 1 || nbr > 8) { goto L1; }
      if (dist_ (nbf, nbr, wf, wr) < 2) { goto L1; }
      if (nbf == pf && nbr == pr) { goto L2; }
      if (nbr == pr + 1 && (nbf == pf - 1 || nbf == pf + 1)) { goto L1; }
      ++nm;
      if (kpkwv_ (pf, pr, wf, wr, nbf, nbr) == 0) { goto L2; }
    L1:
      ;
    }
  if (nm > 0) { ret_val = -1; }
L2:
  return ret_val;
}				/* kpkbv_ */

inline
int
ScoreK1PK (short int side,
	   short int winner,
	   short int loser,
	   short int king1,
	   register short int king2,
	   register short int sq)

     /*
      *  We call Don Beal's routine with the necessary parameters and determine
      *  win/draw/loss.  Then we compute the real evaluation which is +-500 for
      *  a win/loss and 10 for a draw, plus some points to lead the computer to
      *  a decisive winning/drawing line.
      */

{
  short s, win, sqc, sqr, k1c, k1r, k2c, k2r;
  const int drawn = 10, won = 500;

#ifdef CACHE
  if (ProbeEETable(side,&s)) return s;
#endif CACHE

  sqc = column (sq) + 1;
  sqr = row (sq) + 1;
  k1c = column (king1) + 1;
  k1r = row (king1) + 1;
  k2c = column (king2) + 1;
  k2r = row (king2) + 1;
  if (winner == black)
    {
      sqr = 9 - sqr;
      k1r = 9 - k1r;
      k2r = 9 - k2r;
    }
  if (sqc > 4)
    {
      sqc = 9 - sqc;
      k1c = 9 - k1c;
      k2c = 9 - k2c;
    }

  if (side == winner) win = kpkwv_ (sqc, sqr, k1c, k1r, k2c, k2r);
  else                win = kpkbv_ (sqc, sqr, k1c, k1r, k2c, k2r);

  if (!win)  s = drawn + 5 * distance (sq, king2) - 5*distance(king1,king2);
  else       s = won + 50 * (sqr - 2) + 10*distance(sq,king2);

#ifdef CACHE 
  if (PUTVAR) PutInEETable (side, s); 
#endif CACHE 

  return s;
}

inline
int
ScoreKBNK (short int winner, short int king1, short int king2)


/*
 * Score King+Bishop+Knight versus King endings. This doesn't work all that
 * well but it's better than nothing.
 */

{
    register short s, sq, KBNKsq = 0;

    for (sq = 0; sq < 64; sq++)
	if (board[sq] == bishop)
	    KBNKsq = (((row (sq) % 2) == (column (sq) % 2)) ? 0 : 7);

    s = emtl[winner] - 300;
    s += ((KBNKsq == 0) ? KBNK[king2] : KBNK[locn (row (king2), 7 - column (king2))]);
    s -= ((taxicab (king1, king2) + distance (PieceList[winner][1], king2) + distance (PieceList[winner][2], king2)));
    return (s);
}

inline
short int
ScoreLoneKing (short int side)

     /*
      * Static evaluation when loser has only a king and winner has no pawns or no
      * pieces.
      */

{
  register short winner, loser, king1, king2, s, i;

  if (mtl[white] == valueK && mtl[black] == valueK)
    return 0;
  UpdateWeights ();
  winner = ((mtl[white] > mtl[black]) ? white : black);
  loser = winner ^ 1;
  king1 = PieceList[winner][0];
  king2 = PieceList[loser][0];

  s = 0;

  if (pmtl[winner] == 0)
    {
      if (emtl[winner] == valueB + valueN)
	s = ScoreKBNK (winner, king1, king2);
      else if (emtl[winner] == valueN + valueN || 
               emtl[winner] == valueN || emtl[winner] == valueB)
	s = 0; 
      else
	s = 500 + emtl[winner] - DyingKing[king2] - 2 * distance (king1, king2);
    }
  else
    {
      if (pmtl[winner] == valueP)
	s = ScoreK1PK (side, winner, loser, king1, king2, PieceList[winner][1]);
      else
	for (i = 1; i <= PieceCnt[winner]; i++)
	  s += ScoreKPK (side, winner, loser, king1, king2, PieceList[winner][i]);
    }
  return ((side == winner) ? s : -s);
}


int
evaluate (register short int side,
	  register short int ply,
	  register short int alpha,
	  register short int beta,
	  short int INCscore,
	  short int *InChk)	/* output Check flag */

/*
 * Compute an estimate of the score by adding the positional score from the
 * previous ply to the material difference. If this score falls inside a
 * window which is 180 points wider than the alpha-beta window (or within a
 * 50 point window during quiescence search) call ScorePosition() to
 * determine a score, otherwise return the estimated score. If one side has
 * only a king and the other either has no pawns or no pieces then the
 * function ScoreLoneKing() is called.
 */

{
    register short evflag, xside;
    register short int slk;
    short s;

    xside = side ^ 1;
    s = -Pscore[ply - 1] + mtl[side] - mtl[xside] - INCscore;
    hung[white] = hung[black] = 0;
    slk = ((mtl[white] == valueK && (pmtl[black] == 0 || emtl[black] == 0)) ||
	  (mtl[black] == valueK && (pmtl[white] == 0 || emtl[white] == 0)));

	      /* should we use the estimete or score the position */
	if (!slk && (ply == 1 ||
#ifdef CACHE
    	(CheckEETable (side)) ||
#endif
	(Sdepth ==  ply) || 
        (ply > Sdepth && (s >= (alpha - 30) && s <= (beta + 30)) )))
	
      {
	  /* score the position */
	  ataks (side, atak[side]);
	  if (Anyatak (side, PieceList[xside][0])){
	  ataks (xside, atak[xside]);
	  *InChk = Anyatak (xside, PieceList[side][0]);
	      return (10001 - ply);
	  }
	  ataks (xside, atak[xside]);
	  *InChk = Anyatak (xside, PieceList[side][0]);
#ifndef BAREBONES 
	  EvalNodes++;
#endif
	if(ply>4)PUTVAR=true;
	      s = ScorePosition (side);
	PUTVAR=false;
      }
    else
      {
	  /* use the estimate but look at check and slk */
	  *InChk = SqAtakd (PieceList[side][0], xside);
	  if (SqAtakd (PieceList[xside][0], side)){
	      return (10001 - ply);
	  }

	  if (slk) {
	    if (ply>4) PUTVAR=true;
            s = ScoreLoneKing (side);
            PUTVAR=false;
            }
      }

    Pscore[ply] = s - mtl[side] + mtl[xside];
    ChkFlag[ply - 1] = ((*InChk) ? Pindex[TOsquare] + 1 : 0);
    return (s);
}

inline
int
BRscan (register short int sq, short int *mob)

/*
 * Find Bishop and Rook mobility, XRAY attacks, and pins. Increment the
 * hung[] array if a pin is found.
 */
{
    register unsigned char *ppos, *pdir;
    register short s, mobx;
    register short u, pin;
    short piece, *Kf;
    mobx = s = 0;
    Kf = Kfield[c1];
    piece = board[sq];
    ppos = nextpos[piece][sq];
    pdir = nextdir[piece][sq];
    u = ppos[sq];
    pin = -1;			/* start new direction */
    do
      {
	  s += Kf[u];
	  if (color[u] == neutral)
	    {
		mobx++;
		if (ppos[u] == pdir[u])
		    pin = -1;	/* oops new direction */
		u = ppos[u];
	    }
	  else if (pin < 0)
	    {
		if (board[u] == pawn || board[u] == king)
		    u = pdir[u];
		else
		  {
		      if (ppos[u] != pdir[u])
			  pin = u;	/* not on the edge and on to find a pin */
		      u = ppos[u];
		  }
	    }
	  else
	    {
		if (color[u] == c2 && (board[u] > piece || atk2[u] == 0))
		  {
		      if (color[pin] == c2)
			{
			    s += PINVAL;
			    if (atk2[pin] == 0 || atk1[pin] > control[board[pin]] + 1)
				++hung[c2];
			}
		      else
			  s += XRAY;
		  }
		pin = -1;	/* new direction */
		u = pdir[u];
	    }
      }
    while (u != sq);
    *mob = mobx;
    return s;
}

inline
short int
KingScan (register short int sq)

/*
 * Assign penalties if king can be threatened by checks, if squares near the
 * king are controlled by the enemy (especially the queen), or if there are
 * no pawns near the king. The following must be true: board[sq] == king c1
 * == color[sq] c2 == otherside[c1]
 */

#define ScoreThreat \
	if (color[u] != c2)\
  	if (atk1[u] == 0 || (atk2[u] & 0xFF) > 1) ++cnt;\
  	else s -= 3

{
    register short cnt;
    register unsigned char *ppos, *pdir;
    register short int s;
    register short u;
    short int ok;

    s = 0;
    cnt = 0;
    if (HasBishop[c2] || HasQueen[c2])
      {
	  ppos = nextpos[bishop][sq];
	  pdir = nextdir[bishop][sq];
	  u = ppos[sq];
	  do
	    {
		if (atk2[u] & ctlBQ)
		    ScoreThreat;
		u = ((color[u] == neutral) ? ppos[u] : pdir[u]);
	    }
	  while (u != sq);
      }
    if (HasRook[c2] || HasQueen[c2])
      {
	  ppos = nextpos[rook][sq];
	  pdir = nextdir[rook][sq];
	  u = ppos[sq];
	  do
	    {
		if (atk2[u] & ctlRQ)
		    ScoreThreat;
		u = ((color[u] == neutral) ? ppos[u] : pdir[u]);
	    }
	  while (u != sq);
      }
    if (HasKnight[c2])
      {
	  pdir = nextdir[knight][sq];
	  u = pdir[sq];
	  do
	    {
		if (atk2[u] & ctlNN)
		    ScoreThreat;
		u = pdir[u];
	    }
	  while (u != sq);
      }
    s += (KSFTY * KTHRT[cnt]) / 16;

    cnt = 0;
    ok = false;
    pdir = nextpos[king][sq];
    u = pdir[sq];
    do
      {
	  if (board[u] == pawn)
	      ok = true;
	  if (atk2[u] > atk1[u])
	    {
		++cnt;
		if (atk2[u] & ctlQ)
		    if (atk2[u] > ctlQ + 1 && atk1[u] < ctlQ)
			s -= 4 * KSFTY;
	    }
	  u = pdir[u];
      }
    while (u != sq);
    if (!ok)
	s -= KSFTY;
    if (cnt > 1)
	s -= (KSFTY);
    return (s);
}

inline
int
trapped (register short int sq)

/*
 * See if the attacked piece has unattacked squares to move to. The following
 * must be true: c1 == color[sq] c2 == otherside[c1]
 */

{
    register short u;
    register unsigned char *ppos, *pdir;
    register short int piece;

    piece = board[sq];
    ppos = nextpos[ptype[c1][piece]][sq];
    pdir = nextdir[ptype[c1][piece]][sq];
    if (piece == pawn)
      {
	  u = ppos[sq];		/* follow no captures thread */
	  if (color[u] == neutral)
	    {
		if (atk1[u] >= atk2[u])
		    return (false);
		if (atk2[u] < ctlP)
		  {
		      u = ppos[u];
		      if (color[u] == neutral && atk1[u] >= atk2[u])
			  return (false);
		  }
	    }
	  u = pdir[sq];		/* follow captures thread */
	  if (color[u] == c2)
	      return (false);
	  u = pdir[u];
	  if (color[u] == c2)
	      return (false);
      }
    else
      {
	  u = ppos[sq];
	  do
	    {
		if (color[u] != c1)
		    if (atk2[u] == 0 || board[u] >= piece)
			return (false);
		u = ((color[u] == neutral) ? ppos[u] : pdir[u]);
	    }
	  while (u != sq);
      }
    return (true);
}


static inline int
PawnValue (register short int sq, short int side)
/*
 * Calculate the positional value for a pawn on 'sq'.
 */

{
    register short fyle, rank;
    register short j, s, a1, a2, in_square, r, e;

    a1 = (atk1[sq] & 0x4FFF);
    a2 = (atk2[sq] & 0x4FFF);
    rank = row (sq);
    fyle = column (sq);
    s = 0;
    if (c1 == white)
      {
	  s = Mwpawn[sq];
	  if ((sq == 11 && color[19] != neutral)
	      || (sq == 12 && color[20] != neutral))
	      s += PEDRNK2B;
	  if ((fyle == 0 || PC1[fyle - 1] == 0)
	      && (fyle == 7 || PC1[fyle + 1] == 0))
	      s += ISOLANI[fyle];
	  else if (PC1[fyle] > 1)
	      s += PDOUBLED;
	  if (a1 < ctlP && atk1[sq + 8] < ctlP)
	    {
		s += BACKWARD[a2 & 0xFF];
		if (PC2[fyle] == 0)
		    s += PWEAKH;
		if (color[sq + 8] != neutral)
		    s += PBLOK;
	    }
	  if (PC2[fyle] == 0)
	    {
		if (side == black)
		    r = rank - 1;
		else
		    r = rank;
		in_square = (row (bking) >= r && distance (sq, bking) < 8 - r);
		e = (a2 == 0 || side == white)? 0:1;
		for (j = sq + 8; j < 64; j += 8)
		    if (atk2[j] >= ctlP)
		      {
			  e = 2;
			  break;
		      }
		    else if (atk2[j] > 0 || color[j] != neutral)
			e = 1;
		if (e == 2)
		    s += (stage * PassedPawn3[rank]) / 10;
		else if (in_square || e == 1)
		    s += (stage * PassedPawn2[rank]) / 10;
		else if (emtl[black] > 0)
		    s += (stage * PassedPawn1[rank]) / 10;
		else
		    s += PassedPawn0[rank];
	    }
      }
    else if (c1 == black)
      {
	  s = Mbpawn[sq];
	  if ((sq == 51 && color[43] != neutral)
	      || (sq == 52 && color[44] != neutral))
	      s += PEDRNK2B;
	  if ((fyle == 0 || PC1[fyle - 1] == 0) &&
	      (fyle == 7 || PC1[fyle + 1] == 0))
	      s += ISOLANI[fyle];
	  else if (PC1[fyle] > 1)
	      s += PDOUBLED;
	  if (a1 < ctlP && atk1[sq - 8] < ctlP)
	    {
		s += BACKWARD[a2 & 0xFF];
		if (PC2[fyle] == 0)
		    s += PWEAKH;
		if (color[sq - 8] != neutral)
		    s += PBLOK;
	    }
	  if (PC2[fyle] == 0)
	    {
		r = rank + (side == white)?1:0;
		in_square = (row (wking) <= r && distance (sq, wking) < r + 1);
		e = (a2 == 0 || side == black)?0:1;
		for (j = sq - 8; j >= 0; j -= 8)
		    if (atk2[j] >= ctlP)
		      {
			  e = 2;
			  break;
		      }
		    else if (atk2[j] > 0 || color[j] != neutral)
			e = 1;
		if (e == 2)
		    s += (stage * PassedPawn3[7 - rank]) / 10;
		else if (in_square || e == 1)
		    s += (stage * PassedPawn2[7 - rank]) / 10;
		else if (emtl[white] > 0)
		    s += (stage * PassedPawn1[7 - rank]) / 10;
		else
		    s += PassedPawn0[7 - rank];
	    }
      }
    if (a2 > 0)
      {
	  if (a1 == 0 || a2 > ctlP + 1)
	    {
		s += HUNGP;
		if (trapped (sq))
		    hung[c1] += 2;
		hung[c1]++;
	    }
	  else if (a2 > a1)
	      s += ATAKD;
      }
    return (s);
}

inline
int
KnightValue (register short int sq, short int side)

/*
 * Calculate the positional value for a knight on 'sq'.
 */

{
    register short s, a2, a1;

    s = Mknight[c1][sq];
    a2 = (atk2[sq] & 0x4FFF);
    if (a2 > 0)
      {
	  a1 = (atk1[sq] & 0x4FFF);
	  if (a1 == 0 || a2 > ctlBN + 1)
	    {
		s += HUNGP;
		if (trapped (sq))
		    hung[c1] += 2;
		hung[c1]++;
	    }
	  else if (a2 >= ctlBN || a1 < ctlP)
	      s += ATAKD;
      }
    return (s);
}

inline
int
BishopValue (register short int sq, short int side)

/*
 * Calculate the positional value for a bishop on 'sq'.
 */

{
    register short s;
    register short a2, a1;
    short mob;

    s = Mbishop[c1][sq];
    s += BRscan (sq, &mob);
    s += BMBLTY[mob];
    a2 = (atk2[sq] & 0x4FFF);
    if (a2 > 0)
      {
	  a1 = (atk1[sq] & 0x4FFF);
	  if (a1 == 0 || a2 > ctlBN + 1)
	    {
		s += HUNGP;
		if (trapped (sq))
		    hung[c1] += 2;
		hung[c1]++;
	    }
	  else if (a2 >= ctlBN || a1 < ctlP)
	      s += ATAKD;
      }
    return (s);
}

inline
int
RookValue (register short int sq, short int side)

/*
 * Calculate the positional value for a rook on 'sq'.
 */

{
    register short s;
    register short fyle, a2, a1;
    short mob;

    s = RookBonus;
    s += BRscan (sq, &mob);
    s += RMBLTY[mob];
    fyle = column (sq);
    if (PC1[fyle] == 0)
	{ s += RHOPN;
    		if (PC2[fyle] == 0)
			s += RHOPNX;
        }
    if (pmtl[c2] > 100 && row (sq) == rank7[c1])
	s += 10;
    if (stage > 2)
	s += 14 - taxicab (sq, EnemyKing);
    a2 = (atk2[sq] & 0x4FFF);
    if (a2 > 0)
      {
	  a1 = (atk1[sq] & 0x4FFF);
	  if (a1 == 0 || a2 > ctlR + 1)
	    {
		s += HUNGP;
		if (trapped (sq))
		    hung[c1] += 2;
		hung[c1]++;
	    }
	  else if (a2 >= ctlR || a1 < ctlP)
	      s += ATAKD;
      }
    return (s);
}

inline
int
QueenValue (register short int sq, short int side)

/*
 * Calculate the positional value for a queen on 'sq'.
 */

{
    register short s, a2, a1;

    s = ((distance (sq, EnemyKing) < 3) ? 12 : 0);
    if (stage > 2)
	s += 14 - taxicab (sq, EnemyKing);
    a2 = (atk2[sq] & 0x4FFF);
    if (a2 > 0)
      {
	  a1 = (atk1[sq] & 0x4FFF);
	  if (a1 == 0 || a2 > ctlQ + 1)
	    {
		s += HUNGP;
		if (trapped (sq))
		    hung[c1] += 2;
		hung[c1]++;
	    }
	  else if (a2 >= ctlQ || a1 < ctlP)
	      s += ATAKD;
      }
    return (s);
}

inline
int
KingValue (register short int sq, short int side)

/*
 * Calculate the positional value for a king on 'sq'.
 */

{
    register short s;
    register short fyle;
    short int a2, a1;
    s = (emtl[side ^ 1] > KINGPOSLIMIT) ? Mking[c1][sq] : Mking[c1][sq] / 2;
    if (KSFTY > 0)
	if (Developed[c2] || stage > 0)
	    s += KingScan (sq);
    if (castld[c1])
	s += KCASTLD;
    else if (Mvboard[kingP[c1]])
	s += KMOVD;

    fyle = column (sq);
    if (PC1[fyle] == 0)
	s += KHOPN;
    if (PC2[fyle] == 0)
	s += KHOPNX;
    switch (fyle)
      {
      case 5:
	  if (PC1[7] == 0)
	      s += KHOPN;
	  if (PC2[7] == 0)
	      s += KHOPNX;
	  /* Fall through */
      case 4:
      case 6:
      case 0:
	  if (PC1[fyle + 1] == 0)
	      s += KHOPN;
	  if (PC2[fyle + 1] == 0)
	      s += KHOPNX;
	  break;
      case 2:
	  if (PC1[0] == 0)
	      s += KHOPN;
	  if (PC2[0] == 0)
	      s += KHOPNX;
	  /* Fall through */
      case 3:
      case 1:
      case 7:
	  if (PC1[fyle - 1] == 0)
	      s += KHOPN;
	  if (PC2[fyle - 1] == 0)
	      s += KHOPNX;
	  break;
      default:
	  /* Impossible! */
	  break;
      }

    a2 = (atk2[sq] & 0x4FFF);
    if (a2 > 0)
      {
	  a1 = (atk1[sq] & 0x4FFF);
	  if (a1 == 0 || a2 > ctlK + 1)
	    {
		s += HUNGP;
		++hung[c1];
	    }
	  else
	      s += ATAKD;
      }
    return (s);
}




short int
ScorePosition (register short int side)

/*
 * Perform normal static evaluation of board position. A score is generated
 * for each piece and these are summed to get a score for each side.
 */

{
    register short int score;
    register short sq, i, xside;
    short int s;
    short int escore;

    UpdateWeights ();
    xside = side ^ 1;
    hung[white] = hung[black] = pscore[white] = pscore[black] = 0;
#ifdef CACHE
    if (!ProbeEETable (side, &s))
      {
#endif
	  for (c1 = white; c1 <= black; c1++)
	    {
		c2 = c1 ^ 1;
		/* atk1 is array of atacks on squares by my side */
		atk1 = atak[c1];
		/* atk2 is array of atacks on squares by other side */
		atk2 = atak[c2];
		/* same for PC1 and PC2 */
		PC1 = PawnCnt[c1];
		PC2 = PawnCnt[c2];
		for (i = PieceCnt[c1]; i >= 0; i--)
		  {
		      sq = PieceList[c1][i];
		      switch (board[sq])
			{
			case pawn:
			    s = PawnValue (sq, side);
			    break;
			case knight:
			    s = KnightValue (sq, side);
			    break;
			case bishop:
			    s = BishopValue (sq, side);
			    break;
			case rook:
			    s = RookValue (sq, side);
			    break;
			case queen:
			    s = QueenValue (sq, side);
			    break;
			case king:
			    s = KingValue (sq, side);
			    break;
			default:
			    s = 0;
			    break;
			}
		      pscore[c1] += s;
		      svalue[sq] = s;
		  }
	    }
    if (hung[side] > 1)
	pscore[side] += HUNGX;
    if (hung[xside] > 1)
	pscore[xside] += HUNGX;

    score = mtl[side] - mtl[xside] + pscore[side] - pscore[xside] + 10;
    if (dither)
      {
	  if (flag.hash)
	      gsrand (starttime + (unsigned int) hashbd);
	  score += urand () % dither;
      }

    if (score > 0 && pmtl[side] == 0)
	if (emtl[side] < valueR)
	    score = 0;
	else if (score < valueR)
	    score /= 2;
    if (score < 0 && pmtl[xside] == 0)
	if (emtl[xside] < valueR)
	    score = 0;
	else if (-score < valueR)
	    score /= 2;

    if (mtl[xside] == valueK && emtl[side] > valueB)
	score += 200;
    if (mtl[side] == valueK && emtl[xside] > valueB)
	score -= 200;
#ifdef CACHE
    if(PUTVAR)PutInEETable(side,score);
#endif
    return (score);
#ifdef CACHE
}
else {
return s;
}
#endif
}
static inline void
BlendBoard (const short int a[64], const short int b[64], short int c[64])
{
    register int sq, s;
    s = 10 - stage;
    for (sq = 0; sq < 64; sq++)
	c[sq] = ((a[sq] * s) + (b[sq] * stage)) / 10;
}


static inline void
CopyBoard (const short int a[64], short int b[64])
{
    register const short *sqa;
    register short *sqb;

    for (sqa = a, sqb = b; sqa < a + 64;)
	*sqb++ = *sqa++;
}

void
ExaminePosition (void)

/*
 * This is done one time before the search is started. Set up arrays Mwpawn,
 * Mbpawn, Mknight, Mbishop, Mking which are used in the SqValue() function
 * to determine the positional value of each piece.
 */

{
    register short i, sq;
    register short fyle;
    short wpadv, bpadv, wstrong, bstrong, z, side, pp, j, k, val, Pd, rank;
    static short PawnStorm = false;

    ataks (white, atak[white]);
    ataks (black, atak[black]);
    UpdateWeights ();
    HasKnight[white] = HasKnight[black] = 0;
    HasBishop[white] = HasBishop[black] = 0;
    HasRook[white] = HasRook[black] = 0;
    HasQueen[white] = HasQueen[black] = 0;
    for (side = white; side <= black; side++)
	for (i = PieceCnt[side]; i >= 0; i--)
	    switch (board[PieceList[side][i]])
	      {
	      case knight:
		  ++HasKnight[side];
		  break;
	      case bishop:
		  ++HasBishop[side];
		  break;
	      case rook:
		  ++HasRook[side];
		  break;
	      case queen:
		  ++HasQueen[side];
		  break;
	      }
    if (!Developed[white])
	Developed[white] = (board[1] != knight && board[2] != bishop &&
			    board[5] != bishop && board[6] != knight);
    if (!Developed[black])
	Developed[black] = (board[57] != knight && board[58] != bishop &&
			    board[61] != bishop && board[62] != knight);
    if (!PawnStorm && stage < 5)
	PawnStorm = ((column (wking) < 3 && column (bking) > 4) ||
		     (column (wking) > 4 && column (bking) < 3));

    CopyBoard (pknight, Mknight[white]);
    CopyBoard (pknight, Mknight[black]);
    CopyBoard (pbishop, Mbishop[white]);
    CopyBoard (pbishop, Mbishop[black]);
    BlendBoard (KingOpening, KingEnding, Mking[white]);
    BlendBoard (KingOpening, KingEnding, Mking[black]);

    for (sq = 0; sq < 64; sq++)
      {
	  fyle = column (sq);
	  rank = row (sq);
	  wstrong = bstrong = true;
	  for (i = sq; i < 64; i += 8)
	      if (Patak (black, i))
		{
		    wstrong = false;
		    break;
		}
	  for (i = sq; i >= 0; i -= 8)
	      if (Patak (white, i))
		{
		    bstrong = false;
		    break;
		}
	  wpadv = bpadv = PADVNCM;
	  if ((fyle == 0 || PawnCnt[white][fyle - 1] == 0) && (fyle == 7 || PawnCnt[white][fyle + 1] == 0))
	      wpadv = PADVNCI;
	  if ((fyle == 0 || PawnCnt[black][fyle - 1] == 0) && (fyle == 7 || PawnCnt[black][fyle + 1] == 0))
	      bpadv = PADVNCI;
	  Mwpawn[sq] = (wpadv * PawnAdvance[sq]) / 10;
	  Mbpawn[sq] = (bpadv * PawnAdvance[63 - sq]) / 10;
	  Mwpawn[sq] += PawnBonus;
	  Mbpawn[sq] += PawnBonus;
	  if (Mvboard[kingP[white]])
	    {
		if ((fyle < 3 || fyle > 4) && distance (sq, wking) < 3)
		    Mwpawn[sq] += PAWNSHIELD;
	    }
	  else if (rank < 3 && (fyle < 2 || fyle > 5))
	      Mwpawn[sq] += PAWNSHIELD / 2;
	  if (Mvboard[kingP[black]])
	    {
		if ((fyle < 3 || fyle > 4) && distance (sq, bking) < 3)
		    Mbpawn[sq] += PAWNSHIELD;
	    }
	  else if (rank > 4 && (fyle < 2 || fyle > 5))
	      Mbpawn[sq] += PAWNSHIELD / 2;
	  if (PawnStorm)
	    {
		if ((column (wking) < 4 && fyle > 4) || (column (wking) > 3 && fyle < 3))
		    Mwpawn[sq] += 3 * rank - 21;
		if ((column (bking) < 4 && fyle > 4) || (column (bking) > 3 && fyle < 3))
		    Mbpawn[sq] -= 3 * rank;
	    }
	  Mknight[white][sq] += 5 - distance (sq, bking);
	  Mknight[white][sq] += 5 - distance (sq, wking);
	  Mknight[black][sq] += 5 - distance (sq, wking);
	  Mknight[black][sq] += 5 - distance (sq, bking);
	  Mbishop[white][sq] += BishopBonus;
	  Mbishop[black][sq] += BishopBonus;
	  for (i = PieceCnt[black]; i >= 0; i--)
	      if (distance (sq, PieceList[black][i]) < 3)
		  Mknight[white][sq] += KNIGHTPOST;
	  for (i = PieceCnt[white]; i >= 0; i--)
	      if (distance (sq, PieceList[white][i]) < 3)
		  Mknight[black][sq] += KNIGHTPOST;
	  if (wstrong)
	      Mknight[white][sq] += KNIGHTSTRONG;
	  if (bstrong)
	      Mknight[black][sq] += KNIGHTSTRONG;
	  if (wstrong)
	      Mbishop[white][sq] += BISHOPSTRONG;
	  if (bstrong)
	      Mbishop[black][sq] += BISHOPSTRONG;

	  if (HasBishop[white] == 2)
	      Mbishop[white][sq] += 8;
	  if (HasBishop[black] == 2)
	      Mbishop[black][sq] += 8;
	  if (HasKnight[white] == 2)
	      Mknight[white][sq] += 5;
	  if (HasKnight[black] == 2)
	      Mknight[black][sq] += 5;

	  Kfield[white][sq] = Kfield[black][sq] = 0;
	  if (distance (sq, wking) == 1)
	      Kfield[black][sq] = KATAK;
	  if (distance (sq, bking) == 1)
	      Kfield[white][sq] = KATAK;
	  Pd = 0;
	  for (k = 0; k <= PieceCnt[white]; k++)
	    {
		i = PieceList[white][k];
		if (board[i] == pawn)
		  {
		      pp = true;
		      z = i + ((row (i) == 6) ? 8 : 16);
		      for (j = i + 8; j < 64; j += 8)
			  if (Patak (black, j) || board[j] == pawn)
			    {
				pp = false;
				break;
			    }
		      Pd += ((pp) ? 5 * taxicab (sq, z) : taxicab (sq, z));
		  }
	    }
	  for (k = 0; k <= PieceCnt[black]; k++)
	    {
		i = PieceList[black][k];
		if (board[i] == pawn)
		  {
		      pp = true;
		      z = i - ((row (i) == 1) ? 8 : 16);
		      for (j = i - 8; j >= 0; j -= 8)
			  if (Patak (white, j) || board[j] == pawn)
			    {
				pp = false;
				break;
			    }
		      Pd += ((pp) ? 5 * taxicab (sq, z) : taxicab (sq, z));
		  }
	    }
	  if (Pd != 0)
	    {
		val = (Pd * stage2) / 10;
		Mking[white][sq] -= val;
		Mking[black][sq] -= val;
	    }
      }
}

void
UpdateWeights (void)

/*
 * If material balance has changed, determine the values for the positional
 * evaluation terms.
 */

{
    register short s1;

    emtl[white] = mtl[white] - pmtl[white] - valueK;
    emtl[black] = mtl[black] - pmtl[black] - valueK;
    tmtl = emtl[white] + emtl[black];
    s1 = ((tmtl > 6600) ? 0 : ((tmtl < 1400) ? 10 : (6600 - tmtl) / 520));
    if (s1 != stage)
      {
	  stage = s1;
	  stage2 = ((tmtl > 3600) ? 0 : ((tmtl < 1400) ? 10 : (3600 - tmtl) / 220));
	  PEDRNK2B = -15;	/* centre pawn on 2nd rank & blocked */
	  PBLOK = -4;		/* blocked backward pawn */
	  PDOUBLED = -14;	/* doubled pawn */
	  PWEAKH = -4;		/* weak pawn on half open file */
	  PAWNSHIELD = 10 - stage;	/* pawn near friendly king */
	  PADVNCM = 10;		/* advanced pawn multiplier */
	  PADVNCI = 7;		/* muliplier for isolated pawn */
	  PawnBonus = stage;

	  KNIGHTPOST = (stage + 2) / 3;	/* knight near enemy pieces */
	  KNIGHTSTRONG = (stage + 6) / 2;	/* occupies pawn hole */

	  BISHOPSTRONG = (stage + 6) / 2;	/* occupies pawn hole */
	  BishopBonus = BBONUS * stage;

	  RHOPN = 10;		/* rook on half open file */
	  RHOPNX = 4;
	  RookBonus = RBONUS * stage;

	  XRAY = 8;		/* Xray attack on piece */
	  PINVAL = 10;		/* Pin */

	  KHOPN = (3 * stage - 30) / 2;	/* king on half open file */
	  KHOPNX = KHOPN / 2;
	  KCASTLD = 10 - stage;
	  KMOVD = -40 / (stage + 1);	/* king moved before castling */
	  KATAK = (10 - stage) / 2;	/* B,R attacks near enemy king */
	  KSFTY = ((stage < 8) ? (KINGSAFETY - 4 * stage) : 0);

	  ATAKD = -6;		/* defender > attacker */
	  HUNGP = -12;		/* each hung piece */
	  HUNGX = -18;		/* extra for >1 hung piece */
      }
}

