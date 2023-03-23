ZX-Trans allows one to transfer a ZX Spectrum snapshot from a PC to a real ZX Spectrum via a serial interface (using either the RS232 port on the Interface 1 or the built-in serial port on the ZX Spectrum +3/ +2A).

You can load a snapshot into your ZX Spectrum in five easy steps (having first created a boot-strap program, if using the built-in serial port on a +3/+2A model--see below):

1. Create a snapshot using your favourite emulator. ZX-Trans supports all common formats, including Z80, SNA, and SZX.

2. Connect your PC to your ZX Spectrum using a serial cable.

3. If using the ZX Interface 1 then, on the PC, open a command window and type:

   > zxtrans -s <port_name> -b 19200 -i <snapshot>

   ---otherwise, if using the built-in port on the +3/ +2A, type:

   > zxtrans -s <port_name> -b 9600 <snapshot>

4. If using the ZX Interface 1 then, on the ZX Spectrum, type:

   > FORMAT "b", 19200: LOAD *"b"

   ---otherwise load the boot-strap routine by inserting the correct tape or diskette and typing:

   > load "zxtrans.bas"

5. Wait for the snapshot to and have fun.

The figures of 19200 and 9600 are the maximum baud rate for the ZX Interface 1 and built-in serial port on the +3/+2A, respectively. The field <port_name> should be replaced with the platform-specific name of the port to which the serial cable is connected (for example, COM1 on Windows). You may set a lower baud rate (that is, a slower transfer rate) if you encounter transfer problems: the program supports the same baud rates as the ZX Spectrum - that is 50, 110, 300, 600, 1200, 2400, 4800, 9600, and 19200 (ZX Interace 1 only).

All going well, you'll see the screen of the ZX Spectrum fill with pixels from the snapsot (except for a few rows of apparently random data near to the top) and, after around one-to-two minutes the snapshot will be
loaded and will start automatically.

The ZX-Trans program provides a number of options, as follows:

-v 	     	     Verbose output, useful for debugging.
-s <port_name>	     Write to serial port <port_name>
-o <file>            Write output to a file, which can then be transferred separately, using a third-party terminal program such as TeraTerm.
-i		     First transfer Interface 1 boot-strap program. Only include this option if using the ZX Interface 1.

-f<mode>	     Specify transfer mode (0 or 1). Mode 0 (byte-by-byte mode) is the default and should be the most reliable. For some serial interfaces (that is, UART drivers), it may be possible to select Mode 1 (fast mode) for a *slightly* quicker transfer.


Creating a boot-strap program:

Only the ZX Interface 1 has built-in support for loading programs over the serial interface. If you use the built-in serial port on the ZX Spectrum +3/+2A, then you first need to create a boot-strap loader, which you can run (from tape or diskette) each time you want to use ZXTrans.

The boot-strap routine is included in the ZXTrans distribution (ZIP file) as both a TZX tape archive and as a WAV tape audio file. One of these tape formats can be used to load the boot-strap program and machine code into a real Spectrum via the tape/ audio socket -- for example, copy the WAV file onto an MP3 player or use a PC tool [http://www.worldofspectrum.org/utilities.html#tzxtools] to play the TZX file directly from your PC.

You can load the boot-strap program using the TZX or WAV file each time. However, if your Spectrum has a disk drive (e.g. the 3-inch drive on the +3), then you will probably want to copy the boot-strap program onto a diskette for convenience. This can be done using a command sequence such as the following:

> LOAD "t:": SAVE "a:"

> MERGE "zxtrans": SAVE "zxtrans" LINE 10

> LOAD "zxtransc" CODE: SAVE "zxtransc" CODE 16384,995

Note that the machine code of the boot-strap program is located in the display buffer: you must make sure the screen does not clear between loading and saving the machine code. This can be done by chaining the LOAD and SAVE commands together, as above, or by switching to lower-screen editing mode by selecting the 'Screen' option from the Edit menu.


Hints, tips, and troubleshooting:

- If possible, match the host for the snapshot to the target ZX Spectrum. In particular, the ZX Spectrum +3/ +2A may well crash when running a snapshot created on the original ZX Spectrum+ 128k model because of differences in the memory-paging functionality.

- If a snapshot consistently crashes when loaded into a real Spectrum, try creating a snapshot at a different point in the program.

- If transfers are unreliable, try reducing the baud rate or try switching to mode 0 (-f0).

  You should also ensure the sender program (at the PC end) is ready to transmit before you start the receiver program (on the ZX Spectrum end).

  You may also need to set the properties of the serial port on your PC. In particular, the parity bit should be set to 'off', the bit-count should be set to '8', and flow control should be set to 'hardware'.

  You may also need to disable any UART buffer that the serial port (at the PC end) has--for example, set the buffer length to 0. Modern serial interfaces (and standard serial-transfer programs) may not respect hardware control at the per-byte level. If you suspect this to be the case for your serial adaptor, try transfer mode 0.

- 16k programs and games will transfer more quickly that 48k programs. However, to make a 16k snapshot, you need to set your emulator to 16k mode. 128k snapshots take the longest to run and, of course, only work on 128k Spectrum models.

- If you cannot transfer any bytes at all, there may be an issue with your serial cable. Please note that the schematic for the cable presented in the "Sinclair Microdrive and Interface 1" manual is not standard -- the TX port is the input port and the RX port is the output port.

- If you have any questions, comments, or encounter problems, feel free to get in touch - markgbeckett@gmail.com.
