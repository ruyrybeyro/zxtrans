	;; This routine reads a block of binary data into memory from the built-in
	;; serial interface on the ZX Spectrum 128k +3 and +2A models.
	;;
	;; On entry:
	;;   hl = base address for block to be written to
	;;   bc = number of bytes to read
	;;
	;; On exit:
	;;   CF = set if read is successful; reset otherwise

READ_BYTE:	equ 0x3a00	; Address of read function for +3/+2A serial port (in ROM3)

ZXT_LOAD_BLOCK:
	exx
	push hl			; Preserve HL' for return to BASIC
	exx
ZXS_LOOP_2:	
	push hl			; Save current destination address and number of
	push bc			; bytes still to read
ZXS_READ_BYTE:	
	rst 0x08
	db 0x1d 		; Code for RS232 In
	jr nc, ZXS_READ_BYTE	; Try again, if no byte read
	pop bc			; Restore current destination address and number of
	pop hl			; bytes still to read
	ld (hl),a		; Store byte read
	inc hl			; Advance to next address
	dec bc			; Decrement counter
	ld a,b			; Check if done
	or c
	jr nz, ZXS_LOOP_2	; Loop if not
	scf			; Indicates success
	exx
	pop hl			; Restore HL' for return to BASIC
	exx
	ret			; Exit
