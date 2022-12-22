#include <stdio.h>
#include <strings.h>
/* convert fen notation to gnu save format */
char            b[8][8];
char            fn[12],*color;
int             i, j, k, l;
int             r, c;
int		eps;
char           *p, *q;
FILE           *F, *O;
int             wq, wk, bq, bk;
char            in[128];
/*
/*		fen2gnu fen-file > outfile
/*
main(argc,argv)
int argc;
char *argv[];
{
	k = 1;
	F = fopen(argv[1], "r");
	while (fgets(in, 128, F) != NULL) {
		if (strncmp("svfe", in, 4) == 0) {
			wq = wk = bq = bk = 10;
			for (i = 0; i < 8; i++) {
				for (j = 0; j < 8; j++) {
					b[i][j] = '.';
				}
			}
			p = &in[5];
			r = 0;
			c = 0;
			while (*p != ' ') {
				if (isdigit(*p)) {
					c += (*p - '0');
				} else if (*p == '/') {
					r++;
					c = 0;
				} else {
					if(isupper(*p))*p = tolower(*p);
					else *p = toupper(*p);
					b[r][c] = *p;
					c++;
				}
				p++;

			}
			p++;
			if(*p == 'W') color = "White"; else color = "Black";
			p += 2;
			if (*p == 'K') {
				wk = 0;
				p++;
			}
			if (*p == 'Q') {
				wq = 0;
				p++;
			}
			if (*p == 'k') {
				bk = 0;
				p++;
			}
			if (*p == 'q') {
				bq = 0;
				p++;
			}
			while (*p != ' ')
				p++;
			p++;
			q = p;
			while (*q != ' ')
				q++;
			*q = '\0';
			if(*p == 'n')eps = -1; else eps = (*p -'a')*8 + *(p+1)-'0';

		} else if (strncmp("srch", in, 4) == 0) {
			sprintf(fn, "bk%03d", k);
			k++;
			O = fopen(fn, "w");
			fprintf(O, "White computer Black Human 1 eps %d # To move %s correct move %sCastled White false Black false # ep %s\nTimeControl 0 Operator Time 0\nWhite Clock 0000 Moves 0\nBlack Clock 0000 Moves 0\n\n", eps,color,&in[5], p);
			for (r = 0; r < 8; r++) {
				fprintf(O, "%c ", '0' + r);
				for (c = 0; c < 8; c++) {
					fprintf(O, "%c", b[r][c]);
				}
				if (r == 0)
					fprintf(O, " %d 10 10 10 %d 10 10 %d\n", bq, ((bq + bk) > 10) ? 10 : 0, bk);
				else if (r == 7)
					fprintf(O, " %d 10 10 10 %d 10 10 %d\n", wq, ((wq + wk) > 10) ? 10 : 0, wk);
				else
					fprintf(O, " 10 10 10 10 10 10 10 10\n");
			}
			fprintf(O, "  abcdefgh\n\nmove  score depth  nodes  time flags capture color\n");
			fclose(O);
		}
	}
}
