# Copyright 2015 George Beckett, All Rights Reserved
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# -   Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# -   Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

#define _WIN32

CC=gcc
CFLAGS=-g -std=c99 -O3 -I/c/opt/include -I.

LDFLAGS =  -L/c/opt/lib -L. ../libspectrum-8.dll /c/opt/bin/libserialport.dll

EXECUTABLE=../zxtrans
EXECUTABLE3=../zxtrans3
Z80_BIN=../zxtrans_receiver.bin
Z80_BIN_3=../zxtrans_receiver_plus3.bin
ASM=z80asm

zxtrans: zxtrans_sender.o Makefile zxtrans_receiver_plus3.bin zxtrans_receiver_inf1.bin
	$(CC) $(LDFLAGS) -o $(EXECUTABLE) zxtrans_sender.o

zxtrans_sender.o: zxtrans_sender.c Makefile
	$(CC) $(CFLAGS) -c -o zxtrans_sender.o zxtrans_sender.c

zxtrans_receiver_inf1.bin: zxtrans_receiver.asm zxtrans_receiver_inf1.asm zxtrans_reader_inf1.asm zxtrans_receiver_store.asm Makefile
	$(ASM) -o $(Z80_BIN) zxtrans_receiver_inf1.asm

zxtrans_receiver_plus3.bin: zxtrans_receiver.asm zxtrans_receiver_plus3.asm zxtrans_reader_plus3.asm zxtrans_receiver_store.asm Makefile
	$(ASM) -o $(Z80_BIN_3) zxtrans_receiver_plus3.asm

clean:
	rm -rf $(EXECUTABLE) *o *.so

distclean:
	rm -rf $(EXECUTABLE) *o *.so
