    org	10000h  
    jmp Label_Loader_Start

%include    "code/fat12.inc" ; from root comile
BaseOfKernelFile         equ 0x00
OffsetOfKernelFile       equ 0x100000

BaseTmpOfKernelAddr      equ 0x9000
OffsetTmpOfKernelFile    equ 0x0000

MemoryStructBufferAddr   equ 0x7E00

[SECTION gdt]

LABEL_GDT:		    dd	0,0
LABEL_DESC_CODE32:	dd	0x0000FFFF,0x00CF9A00
LABEL_DESC_DATA32:	dd	0x0000FFFF,0x00CF9200

GdtLen	equ	$ - LABEL_GDT
GdtPtr	dw	GdtLen - 1
	dd	LABEL_GDT

SelectorCode32	equ	 LABEL_DESC_CODE32 - LABEL_GDT
SelectorData32	equ	 LABEL_DESC_DATA32 - LABEL_GDT

[SECTION gdt64]
LABEL_GDT64:		dq	0x0000000000000000
LABEL_DESC_CODE64:	dq	0x0020980000000000
LABEL_DESC_DATA64:	dq	0x0000920000000000

GdtLen64	equ	$ - LABEL_GDT64
GdtPtr64	dw	GdtLen64 - 1
		dd	LABEL_GDT64

SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64


[SECTION .s16]
[BITS 16]
Label_Loader_Start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00

    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0200h
    mov cx, 21
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, StartLoaderMessage
    int 10h

    push	ax
	in  al,	92h
	or	al,	00000010b
	out	92h,	al
	pop	ax

    ; order to visite 4GB address (by fs)
	cli

	db	0x66
	lgdt	[GdtPtr]	

	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax

	mov	ax,	SelectorData32
	mov	fs,	ax
	mov	eax,	cr0
	and	al,	11111110b
	mov	cr0,	eax

	sti

    ;search kernel.bin
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
    mov si, KernelFileName
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
    mov si, KernelFileName
    jmp Label_Search_For_LoaderBin


Label_Goto_Next_Sector_In_Root_Dir:
    add word [SectorNo], 1
    jmp Label_Search_In_Root_Dir_Begin

;  display on screen .no loader ..
Label_No_LoaderBin:
    mov ax, 1301h
    mov bx, 008ch
    mov dx, 0300h
    mov cx, 21
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, NoKernelMessage
    int 10h
    jmp $

Label_FileName_Found: ; es:di,DIR_Attr

    push es
    push di

    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0300h
    mov cx, 22
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, FindKernelMessage
    int 10h

    pop di
    pop es

    and di, 0ffe0h
    add di, 01ah
    mov ax, word [es:di]

Label_Read_Kernel:
    push	ax
	push	bx
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

    push ax
    mov ax, BaseTmpOfKernelAddr
    mov es, ax
    mov bx, OffsetTmpOfKernelFile
    mov cl, 1
    pop ax

    push ax
    add ax, RootDirSectors          
    add ax, SectorBalance

    ; es:bx ===> target address of read
    call Func_ReadOneSector


    mov cx, 200h
    mov edi, dword [OffsetOfKernelFileCount]
    mov ax, BaseTmpOfKernelAddr
    mov es, ax
    mov esi, OffsetTmpOfKernelFile

Label_Move_Kernel:
    mov al, byte [es:esi]
    mov byte [fs:edi], al
    inc edi
    inc esi
    loop Label_Move_Kernel

    mov dword [OffsetOfKernelFileCount], edi

    pop  ax
    call Func_GetFATEntry
    cmp ax, 0fffh
    jz Label_Kernel_Loaded
    jmp Label_Read_Kernel

Label_Kernel_Loaded:
    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0600h
    mov cx, 25
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, ReadKernelMessage
    int 10h

    mov ax, 0b800h
    mov gs, ax
    mov ah, 0Fh
    mov al, 'G'
    mov [gs:((80 * 0) + 39) * 2], ax

KillMotor:
    push dx
    mov dx, 03F2h
    mov al, 0
    out dx, al
    pop dx


 ; get memory address 
    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0700h
    mov cx, 26
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, StartGetMemoryMessage
    int 10h

    mov ebx, 0
    mov ax, 0x00
    mov es, ax
    mov di, MemoryStructBufferAddr

Lable_Get_Mem_Struct:
    mov eax, 0x0E820
    mov ecx, 20
    mov edx, 0x534D4150
    int 15h
    jc Lable_Get_Mem_Struct_Falil
    add di, 20
    cmp ebx, 0
    jne Lable_Get_Mem_Struct
    jmp Lable_Get_Mem_Struct_OK

Lable_Get_Mem_Struct_Falil:
    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0700h
    mov cx, 30
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, GetMemoryFailMessage
    int 10h

    jmp $

Lable_Get_Mem_Struct_OK:
    mov ax, 1301h
    mov bx, 000fh
    mov dx, 0800h  ;row 8
    mov cx, 29
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, GetMemoryOKMessage
    int 10h


    mov eax, 0
    mov ax, 80
    mov bx, 12
    mul bx
    mov bx, 2
    mul bx

    mov dword [DisplayPosition], eax
    ;=======	get SVGA information

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0900h		;row 8
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAVBEInfoMessage
	int	10h

	mov	ax,	0x00
	mov	es,	ax
	mov	di,	0x8000
	mov	ax,	4F00h

	int	10h


	cmp	ax,	004Fh

	jz	.KO
	
;=======	Fail

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0A00h		
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoErrMessage
	int	10h

	jmp	$

.KO:

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0A00h		;row 10
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoOKMessage
	int	10h

;=======	Get SVGA Mode Info

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0B00h		;row 11
	mov	cx,	24
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAModeInfoMessage
	int	10h


	mov	ax,	0x00
	mov	es,	ax
	
	mov	si,	0x8000

	mov	cx,	22h;;;;;;;;;;;;;;;;;;
LOOP_Disp_VBE_Info:

	mov	ax,	00h
	mov	al,	byte	[es:si]
	call	Func_DispalyNum
	add	si,	1

	loop	LOOP_Disp_VBE_Info
	
	mov	cx,	0x55aa
	push	ax	
	mov	ax,	00h
	mov	al,	ch
	call	Func_DispalyNum
	mov	ax,	00h
	mov	al,	cl	
	call	Func_DispalyNum	
	pop	ax

    mov	cx,	0xff;;;;;;;;;;;;;;

LABEL_Get_Mode_List:

	add	cx,	1

	cmp	cx,	0x200
	jz	LABEL_Get_Mode_Finish

	mov	ax,	4F01h
	mov	edi,	0x8200
	int	10h

	cmp	ax,	004Fh
	jnz	LABEL_Get_Mode_List

	push	ax	
	mov	ax,	00h
	mov	al,	ch
	call	Func_DispalyNum
	mov	ax,	00h
	mov	al,	cl	
	call	Func_DispalyNum	
	pop	ax

	jmp	LABEL_Get_Mode_List
	
LABEL_Get_Mode_Finish:

	mov	cx,	0x55aa
	push	ax	
	mov	ax,	00h
	mov	al,	ch
	call	Func_DispalyNum
	mov	ax,	00h
	mov	al,	cl	
	call	Func_DispalyNum	
	pop	ax


	mov	cx,	0x117	;;;;;;;;;;;;mode
	mov	ax,	4F01h
	mov	edi,	0x8200
	int	10h

	push	ax	
	mov	ax,	00h
	mov	al,	ch
	call	Func_DispalyNum
	mov	ax,	00h
	mov	al,	cl	
	call	Func_DispalyNum	
	pop	ax

	mov	cx,	0x55aa
	push	ax	
	mov	ax,	00h
	mov	al,	ch
	call	Func_DispalyNum
	mov	ax,	00h
	mov	al,	cl	
	call	Func_DispalyNum	
	pop	ax

	mov	si,	0x8200
	mov	cx,	128
LOOP_Disp_Mode_Info:
	mov	ax,	00h
	mov	al,	byte	[es:si]
	call	Func_DispalyNum
	add	si,	1
	loop	LOOP_Disp_Mode_Info

	jmp	Label_SVGA_Mode_Info_Finish

Label_SET_SVGA_Mode_VESA_VBE_FAIL:
	jmp	$

Label_SVGA_Mode_Info_Finish:
    mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0F00h		;row 15
	mov	cx,	30
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoOKMessage
	int	10h



    ;	set the SVGA mode(VESA VBE)

	mov	ax,	4F02h
	mov	bx,	4117h	;========================mode : 0x180 or 0x143
	int 	10h

	cmp	ax,	004Fh
	jnz	Label_SET_SVGA_Mode_VESA_VBE_FAIL


    ; go to protect mode

    cli

	db	0x66
	lgdt	[GdtPtr]	

;    db 0x66
;    lidt [IDT_POINTER]

	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax

    jmp dword SelectorCode32:GO_TO_TMP_Protect

[SECTION .s32]
[BITS 32]

GO_TO_TMP_Protect:
    ; go to IA-32e
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, 7E00h

    ; test 

    call Func_Test_Support_IA32e
    test eax, eax
    jz Func_Test_Support_IA32e_Fail

    ;	init temporary page table 0x90000
    mov	dword	[0x90000],	0x91007
	mov	dword	[0x90800],	0x91007		

	mov	dword	[0x91000],	0x92007

	mov	dword	[0x92000],	0x000083

	mov	dword	[0x92008],	0x200083

	mov	dword	[0x92010],	0x400083

	mov	dword	[0x92018],	0x600083

	mov	dword	[0x92020],	0x800083

	mov	dword	[0x92028],	0xa00083
	; load GDTR
    db	0x66
	lgdt	[GdtPtr64]
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax

	mov	esp,	7E00h

    ; open PAE

    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ;load cr3

    mov eax, 0x90000
    mov cr3, eax

    ; enable

    mov	ecx,	0C0000080h		;IA32_EFER
	rdmsr

	bts	eax,	8
	wrmsr

    ;=======	open PE and paging

	mov	eax,	cr0
	bts	eax,	0
	bts	eax,	31
	mov	cr0,	eax

	jmp	SelectorCode64:OffsetOfKernelFile

[SECTION .s32lib]
[BITS 32]
Func_Test_Support_IA32e:
    mov eax, 0x80000000
    cpuid
    cmp	eax,	0x80000001
    setnb  al
    jb Func_Test_Support_IA32e_OK
    mov eax, 0x80000001
    cpuid
    bt edx, 29
    setc  al

Func_Test_Support_IA32e_OK:
    movzx eax, al
    ret 

Func_Test_Support_IA32e_Fail:
    jmp $

[SECTION .s16lib]
[BITS 16]
    ;display number in al
Func_DispalyNum:
    push ecx
    push edx
    push edi

    push ax
    mov ax, 0b800h
    mov gs, ax
    pop ax

    mov edi, [DisplayPosition]
    mov ah, 0fh
    mov dl, al
    shr al, 4
    mov ecx, 2

    

Label_Func_DispalyNum_Start:
    and al, 0fh
    cmp al, 9
    ja Label_Func_DispalyNum_1
    add al, '0'
    jmp Label_Func_DispalyNum_2

Label_Func_DispalyNum_1:
    sub al, 0Ah
    add al, 'A'

Label_Func_DispalyNum_2:
    mov [gs:edi], ax
    add edi, 2
    mov al, dl
    loop Label_Func_DispalyNum_Start

    mov [DisplayPosition], edi
  
    pop edi
    pop edx
    pop ecx
    ret



    ;AX    ===> number of FAT chart
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

; tmp idt
IDT:
	times	0x50	dq	0
IDT_END:

IDT_POINTER:
		dw	IDT_END - IDT - 1
		dd	IDT

; tmp variable
DisplayPosition		    dd	 0
RootDirSizeForLoop      dw   RootDirSectors
SectorNo		        dw   0
Odd                     db   0
OffsetOfKernelFileCount	dd	OffsetOfKernelFile          

StartLoaderMessage:       db   "start loader by--yixi"
KernelFileName:           db   "KERNEL  BIN",0
NoKernelMessage:          db   "ERROR:NO KERNEL Found"
FindKernelMessage:        db   "NOTE:KERNEL Be Founded"
ReadKernelMessage:        db   "NOTE:KERNEL Read finished" 
StartGetMemoryMessage:    db   "NOTE:Start Get Memory Info" 
GetMemoryOKMessage:       db   "NOTE:Start Get Memory Success" 
GetMemoryFailMessage:     db   "NOTE:Start Get Memory Fail!!!" 
SetSVGAOKMessage:         db   "NOTE:Set SVGA Success" 
SetSVGAFailMessage:       db   "NOTE:Set SVGA Fail!!!"
StartGetSVGAVBEInfoMessage:	db	"Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage:	db	"Get SVGA VBE Info ERROR"
GetSVGAVBEInfoOKMessage:	db	"Get SVGA VBE Info SUCCESSFUL!"
StartGetSVGAModeInfoMessage:	db	"Start Get SVGA Mode Info"
GetSVGAModeInfoErrMessage:	db	"Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage:	db	"Get SVGA Mode Info SUCCESSFUL!"