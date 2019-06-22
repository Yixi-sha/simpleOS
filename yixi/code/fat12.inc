RootDirSectors  equ   14 ; Sector number of rootDir
;（ BPB_RootEntCnt * 32 + BPB_ BytesPerSec - 1) / BPB_ BytesPerSec = (224 × 32 + 512 - 1) I 512 = 14 。
SectorNumberOfRootDirStart   equ 19
SectorNumberOfFAT1Start      equ 1
SectorBalance                equ 17

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
    