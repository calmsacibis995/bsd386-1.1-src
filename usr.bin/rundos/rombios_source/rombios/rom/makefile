beg_rom	= 6000

rom_p = ..\rom
dk_p = ..\dk

#----------------------------------------

our_link = barby_mk

i9 = int9
m = messgs
i17 = int17
rom = bar
rom_15h = bar15
rom_15hs = bar15s
rom_ini = barbyc
rom_z = barbyz

#----------------------------------------

!IFNDEF vpix
!IFNDEF bsd

!ERROR - define one of the following: VPIX, BSD

!ENDIF
!ENDIF


!IFDEF vpix
rom_n = vrom 
rom_t = barbytl
dk_ex = obv
!ENDIF

!IFDEF bsd
rom_n = brom
rom_t = barbt
dk_ex = obb
!ENDIF

#----------------------------------------
lenja_inc = barby40.asm

$(rom_n).bil: $*.exe $(our_link).exe
   barby_mk $* $(beg_rom)

$(our_link).exe: $(our_link).c
   cl /AC $(our_link).c

!CMDSWITCHES +I
$(rom_n).exe: $(rom).obj $(rom_t).obj $(rom_15h).obj $(rom_15hs).obj \
              $(rom_ini).obj $(dk_p)\$(i9).$(dk_ex) $(dk_p)\$(m).$(dk_ex) \
	      $(dk_p)\$(i17).$(dk_ex) $(rom_z).obj
   link  @<<$(rom_n).lnk
$(**: =+^
+) /m
$*
$*.map;
<<keep

!CMDSWITCHES -I

$(rom).obj: $*.asm  $(lenja_inc) makefile
   masm /DBEGIN_OFFS=$(beg_rom)h $*,,$*;

$(rom_t).obj: $*.asm $(lenja_inc)
   masm $*,,$*;

$(rom_15h).obj: $*.asm $(lenja_inc)
   masm $*,,$*;

$(rom_15hs).obj: $*.asm $(lenja_inc)
   masm $*,,$*;

$(rom_z).obj: $*.asm 
   masm $*,,$*;

$(rom_ini).obj: $*.c comarea.h
	cl /ASu /G2 /Zl /Zp /Gr /Oi /Gs /c /Fc $*.c

#----------------------------------------

!IFDEF bsd
i9_swi = /D BSD
m_swi = /D BSD
i17_swi = /D BSD
!ENDIF

#----------------------------------------

int9_dep = $(dk_p)\kb.h $(dk_p)\kbtab.h $(dk_p)\rom.h
i9_sswi = /c /ASw /Zpl /G2 /Gs /Oi /Fc /Fo$(*B).$(dk_ex)

$(dk_p)\$(i9).$(dk_ex): $(*R).c $(int9_dep)
   cd $(dk_p)
   cl /Zg $*.c >int9func.def
   cl $(i9_sswi) $(i9_swi) $(*B).c
   cd $(rom_p)

#----------------------------------------
dk_sswi = /c /ASw /Zpl /G2 /Ox /Fc /Fo$(*B).$(dk_ex)

$(dk_p)\$(m).$(dk_ex): $(*R).c $(dk_p)\rom.h $(rom_p)\comarea.h
   cd $(dk_p)
   cl /Zg /I $(rom_p) $*.c >msgfunc.def
   cl $(dk_sswi) $(m_swi) /I $(rom_p) $(*B).c
   cd $(rom_p)

#----------------------------------------

$(dk_p)\$(i17).$(dk_ex): $(*R).c $(dk_p)\rom.h
   cd $(dk_p)
   cl $(dk_sswi) $(i17_swi) /I $(rom_p) $(*B).c
   cd $(rom_p)

#----------------------------------------


