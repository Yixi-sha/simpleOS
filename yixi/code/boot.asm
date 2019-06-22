    org 0x7c00

BaseOfStack     equ   0x7c00
BaseofLoader    equ   0x1000 ; cs of loader
OffsetOfLoader  equ   0x00

RootDirSectors  equ   14 ; Sector number of rootDir
;（ BPB_RootEntCnt * 32 + BPB_ BytesPerSec - 1) / BPB_ BytesPerSec = (224 × 32 + 512 - 1) I 512 = 14 。
SectorNumberOfRootDirStart   equ 19
SectorNumberOfFAT1Start      equ 1
SectorBalance                equ 17

    jmp short Label_Start
    nop

    BS_OEMName        db    "YIXIboot"
    BPB_BytesPerSec   dw    512
    BPB_SecPerClus    db    1
    BPB_RsvdSecCnt    dw    1
    BPB_NumFATs       db    2
    BPB_RootEntCnt    dw    224
    BPB_TotSect16     dw    2880
    BPB_Media         db    0xf0
    BPB_FATSz16       dw    9
    BPB_SecPerTrk	  dw    18
	BPB_NumHeads	  dw	2
	BPB_HiddSec	      dd	0
	BPB_TotSec32	  dd	0
	BS_DrvNum	      db	0
	BS_Reserved1	  db	0
	BS_BootSig	      db	29h
	BS_VolID	      dd	0
	BS_VolLab	      db	"boot loader"
	BS_FileSysType	  db	 "FAT12   "

Label_Start :

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BaseOfStack
    mov bp, BaseOfStack

    ;    clear screen
    mov ax, 0600h
    mov bx, 0700h
    mov cx, 0
    mov dx , 0184fh
    int 10h

    ;set focus
    mov ax, 0200h
    mov bx, 0000h
    mov dx, 0000h
    int 10h
    ;  display on screen . Start Booting ..
    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0000h
    mov cx, 18
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, StartBootMessage
    int 10h

    ; reset floppy
    xor ah, ah
    xor dl, dl
    int 13h
;    jmp $

; search loard.bin
    mov word [SectorNo], SectorNumberOfRootDirStart

Label_Search_In_Root_Dir_Begin:
    cmp word [RootDirSizeForLoop], 0
    jz Label_No_LoaderBin
    dec word [RootDirSizeForLoop]
    mov ax, 00h
    mov es, ax
    mov bx, 8000h
    mov ax, [SectorNo]
    mov cl, 1
    call Func_ReadOneSector
    mov si, LoaderFileName
    mov di, 8000h
    cld
    mov dx, 10h

Label_Search_For_LoaderBin:

    cmp dx, 0
    jz Label_Goto_Next_Sector_In_Root_Dir
    dec dx
    mov cx, 11

Label_cmp_FileName:

    cmp cx, 0
    jz Label_FileName_Found 
    dec cx
    lodsb
    cmp al, byte [es:di] 
    jz Label_Go_On
    jmp Label_Different

Label_Go_On:

    inc di
    jmp Label_cmp_FileName

Label_Different:
    and di, 0ffe0h
    add di, 20h
    mov si, LoaderFileName
    jmp Label_Search_For_LoaderBin


Label_Goto_Next_Sector_In_Root_Dir:
    add word [SectorNo], 1
    jmp Label_Search_In_Root_Dir_Begin

;  display on screen .no loader ..
Label_No_LoaderBin:
    mov ax, 1301h
    mov bx, 008ch
    mov dx, 0100h
    mov cx, 17
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, NoLoaderMessage
    int 10h
    jmp $


Label_FileName_Found: ; es:di,DIR_Attr
    
    ; get cluster number
    and di, 0ffe0h
    add di, 01ah
    mov ax, word [es:di]

    ; read data

    push ax
    mov ax, BaseofLoader
    mov es, ax
    pop ax 
    mov bx, OffsetOfLoader


Label_Go_On_Read_data:
    cmp ax, 0fffh
    jz Label_File_Loaded

    push	ax
	push	bx
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

    push ax 
    add ax, RootDirSectors          
    add ax, SectorBalance
    mov cl, 1
    call Func_ReadOneSector
    add bx, [BPB_BytesPerSec]
    pop ax
    call Func_GetFATEntry
    jmp Label_Go_On_Read_data
    
Label_File_Loaded:
;    mov bx, 000fh
;    mov dx, 0100h
;    mov cx, 4
;    mov ax, BaseofLoader
;    mov es, ax
;    mov ax, 1301h
;    mov bp, OffsetOfLoader
;    add bp, 1024
;    int 10h
    jmp BaseofLoader:OffsetOfLoader



    ;AH    ===> number of FAT chart
    ; return ax
Func_GetFATEntry:
    push bp
    mov bp, sp
    push es
    push bx
    push dx
   

    push ax
    mov ax, 00
    mov es, ax
    pop ax
    mov byte [Odd], 0
    mov bx, 3
    mul bx
    mov bx, 2
    div bx   ; ax / bx ax ===> / dx ===>%
    cmp dx, 0
    jz Lable_Even
    mov byte [Odd], 1

Lable_Even:

    xor dx, dx
    mov bx, [BPB_BytesPerSec]
    div bx ; ax / bx ax ===> / dx ===>%
    push dx
    mov bx, 8000h
    add ax, SectorNumberOfFAT1Start
    mov cl, 2
    call Func_ReadOneSector
    pop dx

    add bx, dx
    mov ax, [es:bx]
    cmp byte [Odd], 1
    jnz Label_Even_2
    shr ax, 4

Label_Even_2:
    and ax, 0fffh

    pop dx
    pop bx
    pop es
    pop bp
    ret




    ; read one sector from floppy
    ; ax    ===> start number of sector
    ; cl    ===> number of read
    ; es:bx ===> target address of read
Func_ReadOneSector:
    push bp
    mov  bp, sp
    push dx

    sub esp, 2
    mov byte [bp - 2], cl
    push bx
    mov bl, [BPB_SecPerTrk]
    div bl ; ax / bl al ===> / ah ===>%
    inc ah 
    mov cl, ah ;  cl ===> number of start
    mov dh, al ;  dh ===> number of magnetic track 
    shr al, 1
    mov ch, al ;  ch ===> number of cylinder
    and dh, 1
    pop bx
    mov dl, [BS_DrvNum]
Lable_Go_On_Reading:
    mov ah, 2
    mov al, byte [bp -2]
    int 13h
    jc Lable_Go_On_Reading
    add esp, 2

    pop dx
    pop bp
    ret



; tmp variable
RootDirSizeForLoop dw   RootDirSectors
SectorNo		   dw   0
Odd                db   0

; chars 
StartBootMessage:   db "start Boot by yixi"
NoLoaderMessage:    db "ERROR:NO LOADER Found"
LoaderFileName:     db "LOADER  BIN",0

times 510 - ($ - $$) db 0
dw 0xaa55