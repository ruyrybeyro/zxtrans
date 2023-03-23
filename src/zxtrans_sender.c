/*
   ZX-Trans Sender - allows one to transfer a ZX Spectrum snapshot file
   from a PC to a real ZX Spectrum via RS232 serial interface.

   Copyright 2015 George Beckett. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   -   Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
   -   Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
   OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
   DAMAGE.
*/

#define CODELEN 80 /* Set to (at least) length of Z80 set-state routine */
#define CODEHDR 9 /* Standard ZX Spectrum RS232 header length */
#define SERIAL_TIMEOUT 2000 /* Measured in milliseconds */
#define BANK1 0x7FFD /* Port for horizontal RAM switch */
#define BANKM 0x5B5C /* Record of current horizontal RAM switch
			configuration */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <libspectrum.h>
#include <libserialport.h>
#include <time.h>

libspectrum_byte lowByte(libspectrum_word regPair);
libspectrum_byte highByte(libspectrum_word regPair);
void usage(void);
inline int zxtrans_write_block(struct sp_port *port,	      \
			       const libspectrum_byte *buf,   \
			       size_t count, int serialMode,  \
			       unsigned int timeout_ms);

enum verbosity_level {
  SILENT,
  NORMAL,
  VERBOSE,
  DEBUG
};

int main(int argc, char *argv[]){
  int verbosity=NORMAL;

  int writeToSerial=1;
  char *portName="COM1";
  int serialMode=0;
  int baudRate=9600;
  libspectrum_machine targetPlatform=LIBSPECTRUM_MACHINE_UNKNOWN;
  int if1Compatible=0; /* Determine whether to write IF1 stub as part of
			  output */
  int writeToFile=0;
  char *outputFilename="output.bin";

  int tmpInt=-1;
  FILE *inputSnapshot=NULL;
  FILE *outputBinary=NULL;
  FILE *IF1Leader=NULL;
  char *inputBuffer=NULL;
  
  libspectrum_snap *snapshot=NULL;
  libspectrum_error err;
  libspectrum_id_t bufferType;
  libspectrum_class_t bufferClass;
  const char *libSpectrumVersion;

  struct sp_port_config *pSerialOutConfig;
  enum sp_return sp_err;
  
  int sizeofInputSnapshot=0; /* Length of snapshot file */
  int sizeofLeader=0; /* Size of leader used for IF1 mode */
  int readLen=0;
  int sizeofInputRead=0;
  int option=0;
  
  libspectrum_byte z80mc[CODEHDR+CODELEN];
  char *leaderBuffer=NULL;
  
  /* Parse input arguments */
  while ((option = getopt(argc, argv, "vhs:o:b:f:it:")) != -1) {
    switch (option) {
    case 'h' : /* Help */
      usage();
      exit(0);
    case 'v' : /* Verbose output */
      verbosity = VERBOSE;
      break;
    case 'o' : /* Write output to file */
      writeToFile = 1;
      writeToSerial = 0;
      outputFilename = optarg; 
      break;
    case 's' : /* Write output to serial port */
      writeToSerial = 1;
      portName = optarg; 
      break;
    case 'b' : /* Set baud rate */
      baudRate = atoi(optarg);

      if(verbosity > NORMAL)
	printf("Setting baud rate to %i\n", baudRate);

      break;
    case 'i' : /* Set Interface 1 compatibility */
      if1Compatible=1;

      if(verbosity > NORMAL)
	printf("Compatibility with ZX Interface 1 enabled\n");

      break;
    case 'f' : /* Serial transfer mode */
      serialMode = atoi(optarg);

      if(serialMode < 0 || serialMode>2){
	usage();
	exit(EXIT_FAILURE);
      }

      if(verbosity > NORMAL)
	printf("Serial-transfer mode set to %i\n", serialMode);

      break;
    /* case 't' : /\* Target platform *\/ */
    /*   targetPlatform = atoi(optarg); */

    /*   if(verbosity > NORMAL) */
    /* 	printf("Preparing snapshot for target %i\n", targetPlatform); */

    /*   break; */
    default:
      usage(); 
      exit(EXIT_FAILURE);
    }
  }

  /* Check we have parsed all input options */
  if(optind != argc-1){
    usage();
    exit(EXIT_FAILURE);
  }
  
  /* Initialise libSpectrum library */
  err = libspectrum_init();
  libSpectrumVersion = libspectrum_version();
  
  if(verbosity > NORMAL){
    printf("Using libspectrum Version %s\n", libSpectrumVersion);
    printf("Serial interface is based on libserialport Version %s\n", \
	   SP_PACKAGE_VERSION_STRING);
  }
  
  /* Try to open input snapshot */
  if (NULL == (inputSnapshot = fopen(argv[optind], "rb"))){
    printf("Error opening input file.\n");
    exit(EXIT_FAILURE);
  }

  /* Check size of file, by seeking to end (and then returning to
     beginning) */
  fseek(inputSnapshot, 0, SEEK_END);
  sizeofInputSnapshot = ftell(inputSnapshot);
  fseek(inputSnapshot, 0, SEEK_SET); 

  if(verbosity > NORMAL){
    printf("Input snapshot is %i bytes long\n", sizeofInputSnapshot);
  }
  
  /* Create space for serialised input */
  inputBuffer = malloc(sizeofInputSnapshot*sizeof(char));

  if(NULL == inputBuffer){
    printf("Out of memory.\n");
    fclose(inputSnapshot);
    exit(EXIT_FAILURE);
  }

  /* Read snapshot into buffer */
  sizeofInputRead = fread(inputBuffer,			\
			  sizeof(char),			\
			  sizeofInputSnapshot,		\
			  inputSnapshot);
     
  if (sizeofInputRead != sizeofInputSnapshot){
    printf("Read error: only read %i elements\nError is %i\n",	\
  	   sizeofInputRead, ferror(inputSnapshot));
    free(inputBuffer);
    fclose(inputSnapshot);
    exit(EXIT_FAILURE);
  }
  
  /* Close snapshot */
  fclose(inputSnapshot);

  /* Check it is a snapshot */
  err = \
    libspectrum_identify_file_with_class(&bufferType,	\
					 &bufferClass,	\
					 argv[optind],	\
					 inputBuffer,	\
					 sizeofInputSnapshot);

  if(err != 0){
    printf("Unable to determine file type.\n");
    free(inputBuffer);
    exit(EXIT_FAILURE);
  }

  if(bufferClass != LIBSPECTRUM_CLASS_SNAPSHOT){
    printf("File does not look to be a snapshot.%i\n", bufferClass);
    free(inputBuffer);
    exit(EXIT_FAILURE);
  }
  
  /* Allocate space for snapshot */
  snapshot = libspectrum_snap_alloc();

  err = \
    libspectrum_snap_read(snapshot,				\
			  (libspectrum_byte *) inputBuffer,	\
			  sizeofInputSnapshot,			\
			  LIBSPECTRUM_ID_UNKNOWN,		\
			  argv[optind]);
  
  if(err != 0 ){
    printf("Error populating snapshot.\n");
    free(inputBuffer);
    libspectrum_snap_free(snapshot);
    exit(EXIT_FAILURE);
  }

  /* We are done with serialised buffer */
  free(inputBuffer);

  /* Check it is a 16k or 48k snapshot */
  libspectrum_machine machine = libspectrum_snap_machine(snapshot);

  /* if(targetPlatform == LIBSPECTRUM_MACHINE_UNKNOWN) */
  /*   targetPlatform == machine; */

  /* if(!(machine == LIBSPECTRUM_MACHINE_16 || \ */
  /*      machine == LIBSPECTRUM_MACHINE_48)){ */
  /*   printf("Found snapshot for %s\n", libspectrum_machine_name(machine)); */
  /*   printf("%s %s", "Sorry, this version of zxtrans can only",	\ */
  /* 	   "handle 16k and 48k snapshots.\n"); */
  /*   libspectrum_snap_free(snapshot); */
  /*   exit(EXIT_FAILURE); */
  /* } */
  
  if(verbosity > NORMAL)
    printf("Found snapshot for %s\n", libspectrum_machine_name(machine));
  
  /* Print out Z80 state, as recorded in snapshot */
  if (DEBUG == verbosity){
    printf("A = %X\n", (unsigned) libspectrum_snap_a(snapshot));
    printf("F = %X\n", (unsigned) libspectrum_snap_f(snapshot));
    printf("BC = %X\n", (unsigned) libspectrum_snap_bc(snapshot));
    printf("DE = %X\n", (unsigned) libspectrum_snap_de(snapshot));
    printf("HL = %X\n", (unsigned) libspectrum_snap_hl(snapshot));
    
    printf("A' = %X\n", (unsigned) libspectrum_snap_a_(snapshot));
    printf("F' = %X\n", (unsigned) libspectrum_snap_f_(snapshot));
    printf("BC' = %X\n", (unsigned) libspectrum_snap_bc_(snapshot));
    printf("DE' = %X\n", (unsigned) libspectrum_snap_de_(snapshot));
    printf("HL' = %X\n", (unsigned) libspectrum_snap_hl_(snapshot));
    
    printf("IX = %X\n", (unsigned) libspectrum_snap_ix(snapshot));
    printf("IY = %X\n", (unsigned) libspectrum_snap_iy(snapshot));
    
    printf("I = %X\n", (unsigned) libspectrum_snap_i(snapshot));
    printf("R = %X\n", (unsigned) libspectrum_snap_r(snapshot));
    printf("IFF1 = %X\n", (unsigned) libspectrum_snap_iff1(snapshot));
    printf("IFF2 = %X\n", (unsigned) libspectrum_snap_iff2(snapshot));
    printf("IM = %X\n", (unsigned) libspectrum_snap_im(snapshot));
    printf("SP = %X\n", (unsigned) libspectrum_snap_sp(snapshot));
    printf("PC = %X\n", (unsigned) libspectrum_snap_pc(snapshot));
    printf("memptr = %X\n", (unsigned) libspectrum_snap_memptr(snapshot));
  }
 
  /* Write Z80 state information, as m/c program */
  int pc=0;

  /* z80mc[pc++] = 03; /\* Binary code file *\/ */
  /* z80mc[pc++] = CODELEN; /\* Length of code *\/ */
  /* z80mc[pc++] = 00; */
  /* z80mc[pc++] = 00; /\* Start of code *\/ */
  /* z80mc[pc++] = 0x40; */
  /* z80mc[pc++] = 00; /\* Not used *\/ */
  /* z80mc[pc++] = 00; */
  /* z80mc[pc++] = 00; /\* Not used *\/ */
  /* z80mc[pc++] = 00; */
  
  z80mc[pc++] = 0xF3; /* DI */

  switch(machine) {
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
    z80mc[pc++] = 0x3e; /* ld a, out_128_memoryport */
    z80mc[pc++] = libspectrum_snap_out_128_memoryport(snapshot); 
    z80mc[pc++] = 0x01; /* ld bc, BANK1 */
    z80mc[pc++] = 0xFD;
    z80mc[pc++] = 0x7F;
    z80mc[pc++] = 0xED; /* out (c),a */
    z80mc[pc++] = 0x79;
    break;
  default:
    /* if(LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY && libspectrum_machine_capabilities(targetPlatform)){ */
    /*   if(verbosity > NORMAL) */
    /* 	printf("128k target detected\n", targetPlatform); */

    /*   z80mc[pc++] = 0x3e; /\* ld a, %00100000 *\/ */
    /*   z80mc[pc++] = 0x20; /\* Setting bit 6 blocks further paging *\/ */
    /*   z80mc[pc++] = 0x01; /\* ld bc, BANK1 *\/ */
    /*   z80mc[pc++] = 0xFD;  */
    /*   z80mc[pc++] = 0x7F;  */
    /*   z80mc[pc++] = 0xED; /\* out (c),a *\/ */
    /*   z80mc[pc++] = 0x79; */
    /* } */
    /* else{ */
      z80mc[pc++] = 00; /* NOP */
      z80mc[pc++] = 00; /* NOP */
      z80mc[pc++] = 00; /* NOP */
      z80mc[pc++] = 00; /* NOP */
      z80mc[pc++] = 00; /* NOP */
      z80mc[pc++] = 00; /* NOP */
      z80mc[pc++] = 00; /* NOP */
    /* } */
  }
  
  switch(machine) {
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
    z80mc[pc++] = 0x3e; /* ld a, out_128_memoryport */
    z80mc[pc++] = libspectrum_snap_out_plus3_memoryport(snapshot); 
    z80mc[pc++] = 0x01; /* ld bc, BANK1 */
    z80mc[pc++] = 0xFD;
    z80mc[pc++] = 0x1F;
    z80mc[pc++] = 0xED; /* out (c),a */
    z80mc[pc++] = 0x79;
    break;
  default:
    z80mc[pc++] = 00; /* NOP */
    z80mc[pc++] = 00; /* NOP */
    z80mc[pc++] = 00; /* NOP */
    z80mc[pc++] = 00; /* NOP */
    z80mc[pc++] = 00; /* NOP */
    z80mc[pc++] = 00; /* NOP */
    z80mc[pc++] = 00; /* NOP */
  }
  
  z80mc[pc++] = 0x3E; /* ld a, NN */
  z80mc[pc++] = libspectrum_snap_i(snapshot); 
  z80mc[pc++] = 0xED; /* ld i, a */
  z80mc[pc++] = 0x47;
  z80mc[pc++] = 0x3E; /* ld a, NN */
  z80mc[pc++] = libspectrum_snap_r(snapshot); 
  z80mc[pc++] = 0xED; /* ld r, a */
  z80mc[pc++] = 0x4F;
  
  z80mc[pc++] = 0x21; /* ld hl, NNNN */
  z80mc[pc++] = libspectrum_snap_f_(snapshot);
  z80mc[pc++] = libspectrum_snap_a_(snapshot);

  z80mc[pc++] = 0xE5; /* push hl */
  z80mc[pc++] = 0xF1; /* pop af */
  
  z80mc[pc++] = 0x01; /* ld bc, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_bc_(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_bc_(snapshot));

  z80mc[pc++] = 0x11; /* ld de, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_de_(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_de_(snapshot));

  z80mc[pc++] = 0x21; /* ld hl, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_hl_(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_hl_(snapshot));

  z80mc[pc++] = 0xD9; /* exx */
  z80mc[pc++] = 0x08; /* ex af, af' */

  z80mc[pc++] = 0x21; /* ld hl, NNNN */
  z80mc[pc++] = libspectrum_snap_f(snapshot);
  z80mc[pc++] = libspectrum_snap_a(snapshot);

  z80mc[pc++] = 0xE5; /* push hl */
  z80mc[pc++] = 0xF1; /* pop af */
  
  z80mc[pc++] = 0x01; /* ld bc, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_bc(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_bc(snapshot));

  z80mc[pc++] = 0x11; /* ld de, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_de(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_de(snapshot));

  z80mc[pc++] = 0x21; /* ld hl, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_hl(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_hl(snapshot));

  z80mc[pc++] = 0xDD; /* ld ix, NNNN */
  z80mc[pc++] = 0x21;
  z80mc[pc++] = lowByte(libspectrum_snap_ix(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_ix(snapshot));

  z80mc[pc++] = 0xFD; /* ld iy, NNNN */
  z80mc[pc++] = 0x21;
  z80mc[pc++] = lowByte(libspectrum_snap_iy(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_iy(snapshot));

  switch(libspectrum_snap_im(snapshot)){
  case 0:
    z80mc[pc++] = 0xED; /* IM 0 */
    z80mc[pc++] = 0x46;
    break;
  case 1:
    z80mc[pc++] = 0xED; /* IM 1 */
    z80mc[pc++] = 0x56;
    break;
  case 2:
    z80mc[pc++] = 0xED; /* IM 2 */
    z80mc[pc++] = 0x5E;
    break;
  }
  
  z80mc[pc++] = 0x31; /* ld sp, NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_sp(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_sp(snapshot));

  switch(libspectrum_snap_iff1(snapshot)){
  case 0:
    z80mc[pc++] = 0xF3; /* DI */
    break;
  case 1:
    z80mc[pc++] = 0xFB; /* EI */
    break;
  }

  z80mc[pc++] = 0xC3; /* jp NNNN */
  z80mc[pc++] = lowByte(libspectrum_snap_pc(snapshot));
  z80mc[pc++] = highByte(libspectrum_snap_pc(snapshot));

  /* PC=70 at this point */
  
  /* Store page information */
  z80mc[pc++] = 0x05; /* RAM page at 0x4000--0x7FFF */

  switch(machine) {
  case LIBSPECTRUM_MACHINE_16:
    z80mc[pc++] = 0xFF; /* End */

    break;
  case LIBSPECTRUM_MACHINE_48:
    z80mc[pc++] = 0x02; /* RAM page at 0x8000--0xBFFF */
    z80mc[pc++] = 0x00; /* RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0xFF; /* End */

    break;
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
    z80mc[pc++] = 0x02; /* RAM page at 0x8000--0xBFFF */
    z80mc[pc++] = 0x00; /* First RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0x01; /* First RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0x03; /* First RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0x04; /* First RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0x06; /* First RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0x07; /* First RAM page at 0xC000--0xFFFF */
    z80mc[pc++] = 0xFF; /* End */
  }

  /* PC=79 at this point */
  
  /* If requested, write output to file */
  if(writeToFile){
    if(verbosity>NORMAL){
      printf("Writing output to %s\n", outputFilename);
    }
    
    if(NULL == (outputBinary = fopen(outputFilename,"wb"))){
      printf("Error opening output file %s", outputFilename);
      exit(EXIT_FAILURE);
    }

    if(if1Compatible){
      if(2 == serialMode){
	if (NULL == (IF1Leader = fopen("zxtrans_stub_fast.bin", "rb"))){
	  printf("Error opening IF1 leader file.\n");
	  exit(EXIT_FAILURE);
	}
      }
      else{
	if (NULL == (IF1Leader = fopen("zxtrans_stub.bin", "rb"))){
	  printf("Error opening IF1 leader file.\n");
	  exit(EXIT_FAILURE);
	}
      }

      if(verbosity>NORMAL){
	switch (serialMode) {
	  case 0:
	    printf("Using single-byte IF1 loader\n");
	    break;
	  case 1:
	    printf("Using standard IF1 loader\n");
	    break;
	  case 2:
	    printf("Using high-speed IF1 loader\n");
	    break;
	  }
      }
      
      /* Check size of file, by seeking to end (and then returning to
	 beginning) */
      fseek(IF1Leader, 0, SEEK_END);
      sizeofLeader = ftell(IF1Leader);
      fseek(IF1Leader, 0, SEEK_SET); 

      leaderBuffer = malloc(sizeofLeader*sizeof(char));

      if(NULL == leaderBuffer){
	printf("Out of memory.\n");
	fclose(IF1Leader);
	exit(EXIT_FAILURE);
      }

      /* Read snapshot into buffer */
      sizeofInputRead = fread(leaderBuffer,		\
			      sizeof(char),		\
			      sizeofLeader,		\
			      IF1Leader);
      
      if (sizeofInputRead != sizeofLeader){
	printf("Read error: only read %i elements\nError is %i\n",	\
	       sizeofInputRead, ferror(IF1Leader));
	free(leaderBuffer);
	fclose(IF1Leader);
	exit(EXIT_FAILURE);
      }
    
      /* Close snapshot */
      fclose(IF1Leader);

      /* Write Z80 Set State routine */
      fwrite(leaderBuffer, sizeof(char),	\
	     sizeofLeader, outputBinary);
    }
    
    /* Write Z80 Set State routine */
    fwrite(z80mc, sizeof(libspectrum_byte),	\
	   CODELEN, outputBinary);

    /* Write RAM pages: standard configuration of 16k/ 48k Spectrum uses
       pages 5, 2, and 0 in sequence. */
    /* If not sending screen buffer, then use this version */
    /* fwrite(&libspectrum_snap_pages(snapshot, 5)[6912],	\ */
    /* 	   sizeof(libspectrum_byte), 9472, outputBinary); */
    fwrite(libspectrum_snap_pages(snapshot, 5),	\
    	   sizeof(libspectrum_byte), 0x4000, outputBinary);

    if((machine == LIBSPECTRUM_MACHINE_48) || \
       (machine == LIBSPECTRUM_MACHINE_128) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2A) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS3)) {
      fwrite(libspectrum_snap_pages(snapshot, 2),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
      fwrite(libspectrum_snap_pages(snapshot, 0),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
    }
    
    if((machine == LIBSPECTRUM_MACHINE_128) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2A) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS3)) {
      fwrite(libspectrum_snap_pages(snapshot, 1),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
      fwrite(libspectrum_snap_pages(snapshot, 3),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
      fwrite(libspectrum_snap_pages(snapshot, 4),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
      fwrite(libspectrum_snap_pages(snapshot, 6),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
      fwrite(libspectrum_snap_pages(snapshot, 7),		\
	     sizeof(libspectrum_byte), 0x4000, outputBinary);
    }
    
    /* Close output file */
    fclose(outputBinary);
  }
  
  /* If requested, write output to serial port */
  if(writeToSerial){
    struct sp_port *pSerialPort=NULL;
  
    if((sp_err = sp_get_port_by_name(portName, &pSerialPort)) != SP_OK){
      printf("Error initialising serial port %d\n", sp_err);
      exit(EXIT_FAILURE);
    }

    if((sp_err = sp_open(pSerialPort, SP_MODE_WRITE)) != SP_OK){
      printf("Error opening serial port %d\n", sp_err);
      exit(EXIT_FAILURE);
    }   
    else
      if(verbosity>NORMAL)
	printf("Successfully opened serial port %s\n", portName);
    
    if((sp_err = sp_set_baudrate(pSerialPort, baudRate)) != SP_OK){
      printf("Error setting baud rate of serial port %d\n", sp_err);
      exit(EXIT_FAILURE);
    } 

    if((sp_err = sp_set_parity(pSerialPort, SP_PARITY_NONE)) != SP_OK){
      printf("Error setting parity of serial port\n");
      exit(EXIT_FAILURE);
    }

    if((sp_err = sp_set_bits(pSerialPort, 8)) != SP_OK){
      printf("Error setting bits of serial port\n");
      exit(EXIT_FAILURE);
    }

    if((sp_err = sp_set_stopbits(pSerialPort, 1)) != SP_OK){
      printf("Error setting stop bits of serial port\n");
      exit(EXIT_FAILURE);
    }

    /* sp_err = sp_set_cts(pSerialPort, SP_CTS_FLOW_CONTROL); */
    if((sp_err = sp_set_flowcontrol(pSerialPort, SP_FLOWCONTROL_RTSCTS)) != SP_OK){
      printf("Error setting flow control of serial port\n");
      exit(1);
    }

    if(if1Compatible){
      if(2 == serialMode){
	if (NULL == (IF1Leader = fopen("zxtrans_stub_fast.bin", "rb"))){
	  printf("Error opening IF1 leader file.\n");
	  exit(EXIT_FAILURE);
	}
      }
      else{
	if (NULL == (IF1Leader = fopen("zxtrans_stub.bin", "rb"))){
	  printf("Error opening IF1 leader file.\n");
	  exit(EXIT_FAILURE);
	}
      }

      if(verbosity>NORMAL){
	switch (serialMode) {
	  case 0:
	    printf("Using single-byte IF1 loader\n");
	    break;
	  case 1:
	    printf("Using standard IF1 loader\n");
	    break;
	  case 2:
	    printf("Using high-speed IF1 loader\n");
	    break;
	  }
      }
      
      /* Check size of file, by seeking to end (and then returning to
	 beginning) */
      fseek(IF1Leader, 0, SEEK_END);
      sizeofLeader = ftell(IF1Leader);
      fseek(IF1Leader, 0, SEEK_SET); 

      leaderBuffer = malloc(sizeofLeader*sizeof(char));

      if(NULL == leaderBuffer){
	fclose(IF1Leader);
	printf("Not enough memory to read IF1 Loader stub.\n");
	exit(EXIT_FAILURE);
      }

      /* Read stub code into buffer */
      sizeofInputRead = fread(leaderBuffer,		\
			      sizeof(char),		\
			      sizeofLeader,		\
			      IF1Leader);
      
      if (sizeofInputRead != sizeofLeader){
	printf("Read error: only read %i elements\nError is %i\n",	\
	       sizeofInputRead, ferror(IF1Leader));
	free(leaderBuffer);
	fclose(IF1Leader);
	exit(EXIT_FAILURE);
      }
    
      /* Close IF1 Leader file */
      fclose(IF1Leader);

      /* Write IF1 Leader routine */
      zxtrans_write_block(pSerialPort, leaderBuffer, sizeofLeader, \
			  serialMode, SERIAL_TIMEOUT);
    }
    
    /* Write Z80 Set State routine */
    if(verbosity>NORMAL)
      printf("Writing %d bytes of Z80 Set State information to %s\n", \
	     CODELEN,  portName);

    zxtrans_write_block(pSerialPort, z80mc, CODELEN, \
			serialMode, SERIAL_TIMEOUT);

    if(verbosity>NORMAL)
      printf("Writing memory page 5 information to %s\n", portName);

    /* Increase the baud rate for serialMode=2 */
    if(2 == serialMode){
      if((sp_err = sp_set_baudrate(pSerialPort, 57600)) != SP_OK){
	printf("Error setting baud rate of serial port %d\n", sp_err);
	exit(EXIT_FAILURE);
      } 
    }

    zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 5), \
			0x4000, serialMode, SERIAL_TIMEOUT);
    
    if((machine == LIBSPECTRUM_MACHINE_48) || \
       (machine == LIBSPECTRUM_MACHINE_128) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2A) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS3)) {
      if(verbosity>NORMAL)
	printf("Writing memory page 2 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 2), \
			  0x4000, serialMode, SERIAL_TIMEOUT);

      if(verbosity>NORMAL)
	printf("Writing memory page 0 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 0), \
			  0x4000, serialMode, SERIAL_TIMEOUT);
    }

    if((machine == LIBSPECTRUM_MACHINE_128) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS2A) || \
       (machine == LIBSPECTRUM_MACHINE_PLUS3)) {
      if(verbosity>NORMAL)
	printf("Writing memory page 1 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 1), \
			  0x4000, serialMode, SERIAL_TIMEOUT);

      if(verbosity>NORMAL)
	printf("Writing memory page 3 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 3), \
			  0x4000, serialMode, SERIAL_TIMEOUT);

      if(verbosity>NORMAL)
	printf("Writing memory page 4 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 4), \
			  0x4000, serialMode, SERIAL_TIMEOUT);

      if(verbosity>NORMAL)
	printf("Writing memory page 6 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 6), \
			  0x4000, serialMode, SERIAL_TIMEOUT);

      if(verbosity>NORMAL)
	printf("Writing memory page 7 information to %s\n", portName);

      zxtrans_write_block(pSerialPort, libspectrum_snap_pages(snapshot, 7), \
			  0x4000, serialMode, SERIAL_TIMEOUT);
    }

    

    /* Clean up and close serial port */
    sp_free_config(pSerialOutConfig);
    sp_err = sp_close(pSerialPort);
    sp_free_port(pSerialPort);
  }
  
  /* Exit */
  libspectrum_snap_free(snapshot);
  return 0;
}

inline libspectrum_byte lowByte(libspectrum_word regPair){
  /* Mask out high byte */
  return (libspectrum_byte) (regPair & 0xFF);
}

inline libspectrum_byte highByte(libspectrum_word regPair){
  /* Mask low byte and rotate 8 (binary) places right */
  return (libspectrum_byte) ((regPair &0xFF00)>>8);
}

void usage(void){
  printf("Usage: zxtrans [OPTIONS] <input filename>\n");
  printf(" -o<output filename>\tOutput to file\n");
  printf(" -s<port>\t\tOutput to serial\n");
  printf(" -b<baud>\t\tBaud rate\n");
  printf(" -h\t\t\tPrint this help text\n");
  printf(" -v\t\t\tVerbose mode\n");
  printf(" -i\t\t\tWrite IF1-compatible leader\n");
  printf(" -f<transfer mode>\tTransfer mode:- 0=single-byte; 1=block; 2=fast\n");

  return;
}

inline int zxtrans_write_block(struct sp_port *port,	      \
			       const libspectrum_byte *buf,   \
			       size_t count, int serialMode,  \
			       unsigned int timeout_ms){
  int totalSent = 0;
  int bytesSent = 0;
  enum sp_signal portStatus;
  enum sp_return sp_err;
  
  if(0 == serialMode)
    /* Following manual-control advice noted at
       http://www.worldofspectrum.org/forums/discussion/comment/534124/#Comment_534124 */
    for(int i=0; i<count; i+=2){
      sp_err = sp_set_rts(port, SP_RTS_ON);
      do
	sp_err = sp_get_signals(port, &portStatus);
      while( !(portStatus && SP_SIG_CTS) );

      if (i+1<count)
    	bytesSent = sp_blocking_write(port, &buf[i], 2, timeout_ms);
      else
	bytesSent = sp_blocking_write(port, &buf[i], 1, timeout_ms);
      
      totalSent = totalSent + bytesSent;

      sp_err = sp_set_rts(port, SP_RTS_OFF);
    }
  else
    totalSent = sp_blocking_write(port, buf, count, timeout_ms);
  
  return totalSent;
}
