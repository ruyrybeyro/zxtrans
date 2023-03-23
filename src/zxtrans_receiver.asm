	;; ZX-Trans Receiver (+3 Version)
	;; 
	;; Load ZX Spectrum snapshot, via RS232 serial port, based
	;; on output from zxtrans_sender application.
	;;
	;; 
	;; Copyright 2015 George Beckett, All Rights Reserved
	;; 
	;; Redistribution and use in source and binary forms,
	;; with or without modification, are permitted provided
	;; that the following conditions are met:
	;; 
	;; - Redistributions of source code must retain the above
	;;   copyright notice, this list of conditions and the following
	;;   disclaimer.		
	;; - Redistributions in binary form must reproduce the above
	;;   copyright notice, this list of conditions and the following
	;;   disclaimer in the documentation and/or other materials
	;;   provided with the distribution.
	;; 
	;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
	;; ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
	;; BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	;; AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
	;; EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
	;; INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	;; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
	;; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	;; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	;; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
	;; THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
	;; OF SUCH DAMAGE.
	;;
	;; 
	;; Version history:
	;; Written by George Beckett <markgbeckett@gmail.com>
	;; Version 0.2, Written 25th June 2015
	;; Version 0.3, Written 2nd September 2015 
	;; Version 1.0, Written 17th September 2015 - 48k support
	;; Version 1.1, Written 27th September 2015 - 16k support added
	;;
	;; 
	;;
	;; Program parameters
	;;
ZXT_IF1_ENV_LEN: equ 600	; Length of space for sys var, etc.
ZXT_DISP_LEN: 	equ 6912	; Size of display buffer
ZXT_DISP_SKIP_LEN: equ 1024 	; Number of display bytes to skip
ZXT_16K_LEN:	equ 16384	; No. bytes in 16k RAM
ZXT_48K_LEN:	equ 49152	; No. bytes in 48k RAM
DISPLAY:	equ 0x4000	; Start of display buffer
PRINT_BUFFER:	equ 23296	; Start of print buffer in memory
PROG:		equ 0x5C53	; PROG system variable addr
BANKM:		equ 0x5B5C	; Port for horizontal RAM switches
BANK1:		equ 0x7FFD	; Copy of last value sent to horizontal RAM switch
HEADER_LEN:	equ 9		; Length of standard, binary-block header
STATE_LEN:	equ 80		; Length of Z80 state block
	;; 
	;; Error codes
	;; 
ZXT_OKAY:	equ 00
ZXT_ERR:	equ 01
	;; 
	;; Nine bytes of header information for ZX Spectrum loader
	;; (only used for Interface 1 version)
	;;
	DB 03 			; CODE file
	DW ZXT_END-ZXT_Z80_SET_STATE  	; Length
	DW ZXT_Z80_SET_STATE		; Start
	DW 0x0000		; Not used for CODE
	DW 0x0000		; Not used for CODE
	;;
	;; Locate at 0x0000, for Z80 reset, or at 0x4000 for
	;; start of display buffer
	;; 
	org 0x4000
	;;
	;; Routine can always be called from 0x4000
	;; 
	;; 
	;; Disable interrupts
	;; 
ZXT_Z80_SET_STATE:	
	jp ZXT_START
	ds STATE_LEN-3		; Leave space for customised
	                        ; Z80 set-state routine (loaded
	                        ; separately)
ZXT_START:
	;;
	;; Start by relocating stack, so will not be overwritten
	;; by snapshot
	;;
	pop hl			; Keep return address
	ld (ZXT_RET_ADDR), hl
	ld (ZXT_PREV_SP), sp
	ld sp, ZXT_IF1_ENV - 1
	;;
	;; Load Z80 set-state block
	;; 
	ld hl, 0x4000
	ld bc, STATE_LEN
	call ZXT_LOAD_BLOCK
	;;
	;; Continue if successful
	;;
	jr c, ZXT_CONT_0
	;; 
	;; otherwise return to BASIC
	;; 
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_0:
	;;
	;; Skip early part of display buffer by loading ZXT_DISP_SKIP_LEN
	;; bytes of snapshot into ROM. This approach may not work if, for
	;; any reason, ROM is writable.
	;; 
	ld hl, 0x0000
	ld bc, ZXT_DISP_SKIP_LEN
	call ZXT_LOAD_BLOCK
	;;
	;; Continue if successful
	;;
	jr c, ZXT_CONT_1
	;; 
	;; otherwise return to BASIC
	;; 
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_1:
	;;
	;; Load remainder of display buffer normally
	;;
	ld hl, DISPLAY + ZXT_DISP_SKIP_LEN
	ld bc, ZXT_DISP_LEN - ZXT_DISP_SKIP_LEN
	call ZXT_LOAD_BLOCK
	;;
	;; Continue if successful
	;;
	jr c, ZXT_CONT_2
	;; 
	;; otherwise return to BASIC
	;; 
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_2:
	;;
	;; Work out extent of system variables
	;;
	ld hl, (PROG)
	ld de, PRINT_BUFFER
	and a			; Reset carry flag, ready to subtract
	sbc hl, de 		; Length of system variables
	;;
	;; Check there is sufficient space
	;; 
	ex de, hl		; DE holds calculated length
	ld hl, ZXT_IF1_ENV_LEN	; Space reserved for IF1 env
	and a			; Reset carry flag
	sbc hl, de		; Carry indicates insufficient space
	jr nc, ZXT_CONT_3 	; No carry means there is sufficient space
	;; 
	;; Otherwise return to BASIC
	;; 
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_3:
	;;
	;; Load part of snapshot to be relocated away from system variables
	;;
	ld b,d			; Effectively, ld bc, de
	ld c,e		 	; BC now holds length of block
	push bc
	ld hl, ZXT_IF1_ENV	; Start of reserved space
	call ZXT_LOAD_BLOCK
	pop bc			; Balance stack
	jr c, ZXT_CONT_4
	;; 
	;; Otherwise return to BASIC
	;; 
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_4:	
	;;
	;; Load remainder of first page of snapshot
	;;
	ld hl, ZXT_16K_LEN-ZXT_DISP_LEN
	push bc 		; Length of relocated block
	and a
	sbc hl, bc		; Gives length of remainder
	ld b,h
	ld c,l			; Copy length to BC
	ld hl, (PROG)
	call ZXT_LOAD_BLOCK
	jr c, ZXT_CONT_5
	;; 
	;; Otherwise return to BASIC
	;; 
	pop bc			; Balance stack
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_5:
	;; Check if this is a 16kB snapshot and otherwise
	;; load remainder of snapshot
	ld a, (ZXT_START-9)
	cp 0xFF

	jr z, ZXT_CONT_8
	ld hl, 0x8000
	ld bc, 0x8000
	call ZXT_LOAD_BLOCK
	jr c, ZXT_CONT_6
	;; 
	;; Otherwise return to BASIC
	;; 
	pop bc			; Balance stack
	ld bc, ZXT_ERR
	jp ZXT_EXIT
ZXT_CONT_6:
	;; 
	;; Check if this is a 48k snapshot, otherwise load
	;; remainder of snapshot
	;; 
	ld hl, ZXT_START-7
ZXT_CONT_6A:	
	ld a, (hl)
	cp 0xFF
	jr z, ZXT_CONT_8

	;; Page in next RAM page
	di			; Must disable interupts before paging
	
	ld a,(BANKM)		; Current ROM/ RAM configuration
	and %11111000		; Swap ROM2 and ROM 3, RAM0 and RAM7
	or (hl)
	
	ld (BANKM),a		; Store new value
	ld bc, BANK1		; Port for horiz ROM switching and RAM paging
	out (c),a		; Make change

	ei			; Safe to reenable interupts

	push hl			; Save current page

	ld hl, 0xc000
	ld bc, 0x4000
	call ZXT_LOAD_BLOCK
	pop hl
	jr c, ZXT_CONT_7
	;; 
	;; Otherwise return to BASIC
	;; 
	pop bc
	ld bc, ZXT_ERR
	jp ZXT_EXIT
	
ZXT_CONT_7:	
	;; Advance to next page
	inc hl
	jr ZXT_CONT_6A
	
ZXT_CONT_8:
	;; 
	;; Having finished with ROM routines, initial part
	;; of snapshot can be relocated
	;; 
	pop bc			; Restore length of relocated block
	ld de, PRINT_BUFFER
	ld hl, ZXT_IF1_ENV
	ldir
	;;
	;; Finally set machine state and run
	;;
	jp ZXT_Z80_SET_STATE

ZXT_EXIT:
	;; BC holds exit code
	ld sp,(ZXT_PREV_SP)	; Restore stack pointer
	ld de,(ZXT_RET_ADDR)	; and restore return address
	push de
	ret
