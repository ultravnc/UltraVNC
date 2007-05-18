;***************************************************************************
;*
;*   DLLENTRY.ASM
;*
;*	VER.DLL Entry code
;*
;*	This module generates a code segment called _INIT.
;*	It initializes the local heap if one exists and then calls
;*	the C routine LibMain() which should have the form:
;*	BOOL FAR PASCAL LibMain(HANDLE hInstance,
;*				WORD   wDataSeg,
;*				WORD   cbHeap,
;*				LPSTR  lpszCmdLine);
;*
;*	The result of the call to LibMain is returned to Windows.
;*	The C routine should return TRUE if it completes initialization
;*	successfully, FALSE if some error occurs.
;*
;**************************************************************************

include cmacros.inc

ExternFP <LibMain>               ;The C routine to be called

ifndef SEGNAME
    SEGNAME equ <_TEXT>         ; default seg name
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE


sBegin	CodeSeg          ; this defines what seg this goes in
assumes cs,CodeSeg

?PLM=0                           ;'C'naming
ExternA  <_acrtused>             ;Ensures that Win DLL startup code is linked

?PLM=1                           ;'PASCAL' naming
ExternFP <LocalInit>             ;Windows heap init routine

cProc   LibEntry, <PUBLIC,FAR>   ;Entry point into DLL
cBegin
    push    di               ;Handle of the module instance
    push    ds               ;Library data segment
    push    cx               ;Heap size
    push    es               ;Command line segment
    push    si               ;Command line offset

    ;** If we have some heap then initialize it
    jcxz    callc            ;Jump if no heap specified

    ;** Call the Windows function LocalInit() to set up the heap
    ;**	LocalInit((LPSTR)start, WORD cbHeap);

    xor     ax,ax

    push    ds
    push    ax
    push    cx
    call    LocalInit

    test    ax,ax            ;Did it do it ok ?
    jz      error            ;Quit if it failed

    ;** Invoke the C routine to do any special initialization

callc:
    call    LibMain          ;Invoke the 'C' routine (result in AX)
    jmp     exit

error:
	pop	    si		 ;Clean up stack on a LocalInit error
    pop     es
    pop     cx
    pop     ds
    pop     di
exit:
cEnd

sEnd

END LibEntry
