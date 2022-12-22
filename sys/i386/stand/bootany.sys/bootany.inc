Numeric         equ     30H             ; First numeric value
BootLocation    equ     7C00h           ; Address where DOS loads us
BootSeg         equ     07C0h           ; Segment where DOS loads us
NewBootLocation equ     7A00h           ; Address where we relocate
NewBootSeg      equ     07A0h           ; Segment where we relocate
PartAddr        equ     1beh            ; offset to partition table
KeyboardFlags   equ     417h            ; Address of keyboard mask
NumLockOff      equ     11011111b       ; Mask to turn numlock off
NumLockOn       equ     11111111b       ; Mask to leave numlock on
max_partitions  equ     3               ; Can't fit any more
part_text_len   equ     15              ; max bytes for partition desc

PartitionEntry  struc
BootIndicator   db      ?
BeginHead       db      ?
BeginSector     db      ?
BeginCyl        db      ?
SystemID        db      ?
EndHead         db      ?
EndSector       db      ?
EndCyl          db      ?
RelSectorLow    dw      ?
RelSectorHigh   dw      ?
NumSectorsLow   dw      ?
NumSectorsHigh  dw      ?
PartitionEntry  ends

PartData        struc
partition       db   0
text            db   part_text_len dup(0)
term            db   +80h
PartData        ends

PartDataLen     equ  SIZE PartData * max_partitions

BootData        struc
numlockMask     db   NumLockOff
BootData        ends

BootDataLen     equ  SIZE BootData

DataAddr        equ  PartAddr - BootDataLen - PartDataLen
