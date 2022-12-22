% psgraph.pro -- included prolog for PS graph files
% based on lib/psplot.pro, Copyright 1984 Adobe Systems, Inc.
% $Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/psgraph.pro,v 1.1 1994/01/05 20:06:03 polk Exp $
save 500 dict begin /psgraph exch def
/StartPSGraph
   {newpath 0 0 moveto 0.6 setlinewidth 0 setgray 1 setlinecap
    /imtx matrix currentmatrix def 
    /fnt /Times-Roman findfont def /fontsize 10 def
    72 72 scale
    /ex 72 nail def /ey 720 nail def
    /smtx matrix def fnt fontsize scalefont setfont}bind def
/len{dup mul exch dup mul add sqrt}bind def
/nail{0 dtransform len 0 idtransform len}bind def
/ljust{0 fontsize -3 div rmoveto}bind def
/rjust{dup stringwidth pop neg fontsize -3 div rmoveto}bind def
/cjust{dup stringwidth pop -2 div fontsize -3 div rmoveto}bind def
/vjust{90 rotate /cjust load exec}bind def
/prnt{dup stringwidth pop 6 add /tx exch def /ty fontsize 5 add def
    currentpoint /toy exch def /tox exch def 1 setgray
    newpath
      tox 3 sub toy 5 sub moveto 0 ty rlineto tx 0 rlineto 0 ty neg rlineto
    closepath fill tox toy moveto
    currentColor fc
    show}bind def
/m{newpath moveto}bind def
/s{pts stroke ins}bind def
/fl{pts fill ins}bind def
/n{lineto currentpoint s moveto}bind def
/nf{lineto}bind def
/l{moveto lineto currentpoint s moveto}bind def
/pts{smtx currentmatrix pop imtx setmatrix}bind def
/ins{smtx setmatrix}bind def
/t{pts load exec show ins}bind def
/w{pts load exec prnt ins}bind def
/e{pts ex ey moveto prnt prnt /ey ey 12 sub def ins}bind def
/gs{gsave}bind def
/gr{grestore}bind def
/c{gs newpath 0 0 3 -1 roll 0 360 arc s gr}bind def
/cf{gs newpath 0 0 3 -1 roll 0 360 arc fl gr}bind def
/f{dup lineStyles exch known
    {lineStyles begin load exec setdash end}
    {gs C 20 string cvs (No such line style: ) e gr unC/solid f}ifelse}bind def
/fc{dup dup /currentColor exch store lineColors exch known
    {lineColors begin load exec aload pop setrgbcolor end}
    {gs C 20 string cvs (No such psgraph color: ) e gr unC/black fc}ifelse}bind def
/fw{setlinewidth} def
/C{/Courier findfont 10 scalefont setfont 
    /sfs fontsize def /fontsize 10 def}bind def
/unC{/fontsize sfs def}bind def
/EX{/exec load}bind def
/EndPSGraph{clear psgraph end restore}bind def
/lineStyles 10 dict def lineStyles begin
/solid{{}0}bind def
/dotted{[2 nail 5 nail ] 0}bind def
/longdashed{[10 nail] 0}bind def
/shortdashed{[6 nail] 0}bind def
/dotdashed{[2 nail 6 nail 10 nail 6 nail] 0}bind def
/dashed{/shortdashed load exec}bind def
%/off{[0 100] 0}bind def
%/none{/off load exec}bind def
end
/currentColor 0 def
/lineColors 10 dict def lineColors begin
/red {[1 0 0]}bind def
/blue {[0 0 1]}bind def
/magenta {[1 0 1]}bind def
/green {[0 1 0]}bind def
/black {[0 0 0]}bind def
/cyan {[0 1 1]}bind def
/yellow {[1 1 0]}bind def
/gray {[.5 .5 .5]}bind def
/orange {[1 .66 0]}bind def
/violet {[1 0 1]}bind def
/currentColor /black store
end

/markers 20 dict def markers begin
/none{}bind def
/off{}bind def
/square{0 0 m 0 1 n 1 1 n 1 -1 n -1 -1 n -1 1 n 0 1 n s}bind def
/diamond{0 0 m 0 1.41 n 1.41 0 n 0 -1.41 n -1.41 0 n 0 1.41 n s}bind def
/triangle{0 0 m 0 1 n 1 -0.73 n -1 -0.73 n 0 1 n s}bind def
/up{triangle}bind def
/down{0 0 m 0 -1 n 1 0.73 n -1 0.73 n 0 -1 n s}bind def
/right{0 0 m 1 0 n -0.73 1 n -0.73 -1 n 1 0 n s}bind def
/left{0 0 m -1 0 n 0.73 1 n 0.73 -1 n -1 0 n s}bind def
/circle{0 1 m 0 0 n 1 c s}bind def
/x{1 1 m -1 -1 n -1 1 m 1 -1 n s}bind def
/plus{0 1 m 0 -1 n -1 0 m 1 0 n s}bind def
/cross{plus}bind def
/circle-x{1 c .707 .707 m -.707 -.707 n -.707 .707 m .707 -.707 n s}bind def
/circle-plus{1 c 0 1 m 0 -1 n -1 0 m 1 0 n s}bind def
/filledsquare{0 1 m 1 1 nf 1 -1 nf -1 -1 nf -1 1 nf 0 1 nf fl}bind def
/filleddiamond{0 1.41 m 1.41 0 nf 0 -1.41 nf -1.41 0 nf 0 1.41 nf fl}bind def
/filledtriangle{0 1 m 1 -0.73 nf -1 -0.73 nf 0 1 nf fl}bind def
/filledcircle{0 0 m 1 cf}bind def
/marker{}def
end
/ms 1.0 def
/mg 0 def
/sm{dup markers exch known
    {markers begin load /marker exch def end}
    {gs C 20 string cvs (No such marker type: )e gr unC/off sm}ifelse}bind def
/mk{gs translate /solid f
    mg 0.0 ne {mg setgray} if
    ms .04 mul dup scale markers begin marker end gr}bind def
% end fixed prolog
