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
	;; Space for temporary stack
ZXT_RET_ADDR:	db 0x00, 0x00	; Store for return addr from stack
ZXT_PREV_SP:	db 0x00, 0x00  	; Store for previous stack pointer
	;; 
ZXT_STACK:	DS 0x40
	;; Space for memory that would overwrite system variables
	;; and other state information used by ROM routines
ZXT_IF1_ENV:	DS ZXT_IF1_ENV_LEN
	;; 
	;; Program variables and state
	;; 
ZXT_END:	
