;============================================================================
;
;   HEAP.ASM
;
;   This has code for allocating memory from the default data segment and 
;	returning far pointers to it. It is for 16-bit code only.  
;
;============================================================================

include CMACROS.INC

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif
.386
 .model flat

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
assumes cs,CodeSeg

; Pascal calling convention
?PLM=1

ExternFP    <LocalAlloc>
ExternFP    <LocalFree>
ExternFP	<LocalReAlloc>

LMEM_FIXED      equ 0000h
LMEM_ZEROINIT   equ 0040h

;============================================================================
;
;   HeapAlloc()
;
;   Allocates a pointer out of a heap.  Returns a FAR pointer to it,
;   the selector in DX, if succeeds.
;
;============================================================================
cProc HeapAlloc, <PUBLIC, NEAR>, <DS>
    ParmW   hheapA
    ParmW   cbSize
cBegin
    mov     ax, hheapA
    mov     ds, ax

    push    LMEM_FIXED or LMEM_ZEROINIT
    push    cbSize
    call    LocalAlloc

    ; If this returned non-zero, return hHeap:pLocalPtr in DX:AX
    ; Otherwise return 0:0
    xor     dx, dx
    test    ax, ax
    jz      @F
    mov     dx, hheapA
@@:
cEnd


;============================================================================
;
;   HeapFree()
;
;   Frees a pointer allocated from a heap.  The heap handle is also the
;   selector of the 16:16 pointer.  God, I love 16-bit code sometimes.
;
;============================================================================
cProc HeapFree, <PUBLIC, NEAR>, <DS>
    ParmD   lpvAlloc
cBegin
    mov     ax, seg_lpvAlloc
    mov     ds, ax

    push    off_lpvAlloc
    call    LocalFree
cEnd


sEnd

END
