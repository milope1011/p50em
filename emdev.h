/* emdev.h, Jim Wilcoxson, April 17, 2005
   Device handlers for pio instructions.  Use devnull as a template for
   new handlers.
   NOTES:

   OCP instructions never skip
   SKS instructions skip on specified conditions
   INA/OTA instructions skip if they succeed (data was read/written)

   Device numbers:
   '01 = paper tape reader
   '02 = paper tape punch
   '03 = 1st MPC (line printer/card reader/card punch)
   '04 = SOC board (system console/user terminal)
   '05 = 2nd MPC (line printer/card reader/card punch)
   '06 = card punch? (RTOS User Guide, A-1) / IPC (Engr Handbook p.101)
   '07 = PNC
   '12 = diskette
   '13 = 2nd magtape controller
   '14 = 1st magtape controller
   '20 = control panel / real-time clock
   '21 = 1st 4002 disk controller
   '22 = obsolete: fixed head disk
   '23 = obsolete: 30MB disk
   '24 = tag monitor (tmain.pma) / writable control store
   '25 = obsolete: moving head disk
   '26 = 1st disk controller
   '27 = 2nd disk controller
   '30-32 = BPIOC #1-3 (RTOS User Guide, A-1)
   '33 = 1st Versatec (verdim)
   '34 = 2nd Versatec
   '35 = 4th AMLC
   '36-37 = ELFBUS #1 & 2
   '40 = A/D converter type 6000
   '41 = digital input type 6020
   '42 = digital input #2
   '43 = digital output type 6040
   '44 = digital output #2
   '45 = D/A converter type 6060 (analog output)
   '50 = 1st HSSMLC (cs/slcdim.pma)
   '51 = 2nd HSSMLC
   '52 = 3rd AMLC
   '53 = 2nd AMLC 
   '54 = 1st AMLC
   '55 = MACI autocall unit
   '56 = old SMLC (RTOS User Guide, A-1)
   '60-67 = reserved for user devices
   '70-'73 = Megatek graphics terminals

   Devices emulated by Primos in ks/ptrap.ftn:
     '04 = console, '01 = paper tape reader, '02 = paper tape punch, 
     '20 = control panel
*/

#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

/* this macro is used when I/O is successful.  In VI modes, it sets
   the EQ condition code bit.  In SR modes, it does a skip */

#define IOSKIP \
  if (crs[KEYS] & 010000) \
    crs[KEYS] |= 0100; \
  else \
    RPL++

/* macro to detect if an I/O instruction is followed by JMP *-1 (SR modes)
   or BCNE *-2 (VI modes) */

#define BLOCKIO \
  (get16(RP) == 03776 || (get16(RP) == 0141603 && get16(RP+1) == RPL-2))


void devnull (short class, short func, short device) {

  switch (class) {

  case 0:
    fprintf(stderr," OCP '%02o%02o\n", func, device);
    if (func == 0) {
      ;
    } else {
      fprintf(stderr," unimplemented OCP device '%02o function\n", device);
      exit(1);
    }
    break;

  case 1:
    fprintf(stderr," SKS '%02o%02o\n", func, device);
    if (func == 0)
      IOSKIP;                     /* assume it's always ready */
    else {
      fprintf(stderr," unimplemented SKS device '%02o function\n", device);
      exit(1);
    }
    break;

  case 2:
    fprintf(stderr," INA '%02o%02o\n", func, device);
    if (func == 0) {
      ;
    } else {
      fprintf(stderr," unimplemented INA device '%02o function\n", device);
      exit(1);
    }
    break;

  case 3:
    fprintf(stderr," OTA '%02o%02o\n", func, device);
    if (func == 0 | func == 1) {
      IOSKIP;                     /* OTA '0004 always works on Unix */
    } else {
      fprintf(stderr," unimplemented OTA device '%02o function\n", device);
      exit(1);
    }
    break;
  }
}


/* Device '4: system console

   OCP '0004 = initialize for input only, echoplex, 110 baud, 8-bit, no parity
   OCP '0104 = same, but for output only
   OCP '0204 = set receive interrupt mask
   OCP '0304 = enable receive DMA/C
   OCP '0404 = reset receive interrupt mask and DMA/C enable
   OCP '0504 = set transmit interrupt mask
   OCP '0604 = enable transmit DMA/C
   OCP '0704 = reset transmit interrupt mask and DMA/C enable
   OCP '1004 = Full duplex; software must echo
   OCP '1104 = output a sync pulse (diagnostics) 
   OCP '1204 = Prime normal, independent xmit and recv w/echoplex
   OCP '1304 = Self test mode (internally connects transmitter to receiver)
   OCP '1504 = Set both xmit & rcv interrupt masks
   OCP '1604 = Reset both xmit & rcv interrupt masks
   OCP '1704 = same as '0004, but clears interrupt masks and dma/c enables
               (this is the state after Master Clear)

   SKS '0004 = skip if either receive or xmit ready, whichever are enabled
   SKS '0104 = skip if not busy
   SKS '0204 = skip if receiver not interrupting
   SKS '0304 = skip if control registers valid
   SKS '0404 = skip if neither xmit nor recv are interrupting
   SKS '0504 = skip if xmit not interrupting
   SKS '0604 = skip is xmit ready (can accept a character)
   SKS '0704 = skip if recv ready (character present)
   SKS '1104-'1404 = skip if input bit 1/2/3/4 marking
   SKS '1504 = skip if parity error
   SKS '1604 = skip if character overrun
   SKS '1704 = skip if framing error

   INA '0004 = "or" character into right byte of A, not modifying left byte
   INA '0404 = input receive control register 1 (always works)
   INA '0504 = input receive control register 2 (always works)
   INA '0604 = input xmit control register 1 (always works)
   INA '0704 = input xmit control register 2 (always works)
   INA '1004 = like '0004, but clear A first
   INA '1104 = input device ID
   INA '1404 = input recv DMA/C channel address (these last 4 always work)
   INA '1504 = input xmit DMA/C channel address
   INA '1604 = input recv interrupt vector
   INA '1704 = input xmit interrupt vector

   OTA '0004 = xmit character from A; fails if inited for recv only
   OTA '0404 = output rcv CR 1
   OTA '0504 = output rcv CR 2
   OTA '0604 = output xmit CR 1
   OTA '0704 = output xmit CR 2
   OTA '1404 = output recv DMA/C channel address (these last 4 always work)
   OTA '1504 = output xmit DMA/C channel address
   OTA '1604 = output recv interrupt vector
   OTA '1704 = output xmit interrupt vector

*/

void devasr (short class, short func, short device) {

  static int ttydev=-1;
  static int ttyflags;
  fd_set readfds;
  struct timeval timeout;
  unsigned char ch;
  int newflags;
  int n;

  if (ttydev < 0) {
    ttydev = open("/dev/tty", O_RDWR, 0);
    if (ttydev < 0) {
      perror(" error opening /dev/tty");
      exit(1);
    }
    if (fcntl(ttydev, F_GETFL, ttyflags) == -1) {
      perror(" unable to get tty flags");
      exit(1);
    }
    FD_ZERO(&readfds);
    FD_SET(ttydev, &readfds);
  }

  switch (class) {

  case 0:
    fprintf(stderr," OCP '%02o%02o\n", func, device);
    break;

  case 1:
    fprintf(stderr," SKS '%02o%02o\n", func, device);
    if (func < 7)
      IOSKIP;                     /* assume it's always ready */
    else if (func == 7) {         /* skip if received a char */
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      if (select(1, &readfds, NULL, NULL, &timeout) == 1)
	IOSKIP;
    }
    break;

  case 2:
    fprintf(stderr," INA '%02o%02o\n", func, device);
    if (func == 0 || func == 010) {
      if (BLOCKIO)
	newflags = ttyflags & ~O_NONBLOCK;
      else
	newflags = ttyflags | O_NONBLOCK;
      if (newflags != ttyflags && fcntl(ttydev, F_SETFL, newflags) == -1) {
	perror(" unable to set tty flags");
	exit(1);
      }
      ttyflags = newflags;
      n = read(ttydev, &ch, 1);
      if (n < 0) {
	if (errno != EAGAIN) {
	  perror(" error reading from tty");
	  exit(1);
	}
      } else if (n == 1) {
	if (func >= 010)
	  crs[A] = 0;
	crs[A] = crs[A] | ch | 0x80;
	fprintf(stderr," character read=%o: %c\n", crs[A], crs[A]);
	IOSKIP;
      } else  if (n != 0) {
	fprintf(stderr," unexpected error reading from tty, n=%d", n);
	exit(1);
      }
    } else if (func == 011) {    /* read device id? */
      crs[A] = 4;
      IOSKIP;
    } else if (func == 012) {    /* read control word */
      crs[A] = 04110;
      IOSKIP;
    } else {
      fprintf(stderr," unimplemented INA '04 function\n");
      exit(1);
    }
    break;

  case 3:
    fprintf(stderr," OTA '%02o%02o\n", func, device);
    if (func == 0) {
      ch = crs[A] & 0x7f;
      fprintf(stderr," char to write=%o: %c\n", crs[A], ch);
      if (ch > 0) {
	putchar(crs[A] & 0x7f);
	fflush(stdout);
      }
      IOSKIP;                     /* OTA '0004 always works on Unix */
    } else if (func == 1) {       /* write control word */
      IOSKIP;
    } else {
      fprintf(stderr," unimplemented OTA '04 function\n");
      exit(1);
    }
    break;
  }
}




/* Device '14 - magtape controller #1
 */

void devmt (short class, short func, short device) {

  switch (class) {

  case 0:
    fprintf(stderr," OCP '%02o%02o\n", func, device);
    break;

  case 1:
    fprintf(stderr," SKS '%02o%02o\n", func, device);
    break;

  case 2:
    fprintf(stderr," INA '%02o%02o\n", func, device);
    if (BLOCKIO) {
      fprintf(stderr," Device not supported, so I/O hangs\n");
      exit(1);
    }
    break;

  case 3:
    fprintf(stderr," OTA '%02o%02o\n", func, device);
    if (BLOCKIO) {
      fprintf(stderr," Device not supported, so I/O hangs\n");
      exit(1);
    }
    break;
  }
}


/* Device '20: control panel switches and lights

   OTA '1720 = write to lights (sets CP fetch address)
   INA '1420 = read location from CP ROM (not implemented, used to boot)
   INA '1620 = read control panel switches
*/

void devcp (short class, short func, short device) {

  switch (class) {

  case 0:
    fprintf(stderr," OCP '%02o%02o\n", func, device);
    fprintf(stderr," unimplemented OCP device '%02o function\n", device);
    exit(1);
    break;

  case 1:
    fprintf(stderr," SKS '%02o%02o\n", func, device);
    fprintf(stderr," unimplemented SKS device '%02o function\n", device);
    exit(1);
    break;

  case 2:
    fprintf(stderr," INA '%02o%02o\n", func, device);
    if (func == 016) {
      crs[A] = sswitch;
    } else {
      fprintf(stderr," unimplemented INA device '%02o function\n", device);
      exit(1);
    }
    break;

  case 3:
    fprintf(stderr," OTA '%02o%02o\n", func, device);
    if (func == 017) {           /* write lights */
      IOSKIP;
    } else {
      fprintf(stderr," unimplemented OTA device '%02o function\n", device);
      exit(1);
    }
    break;
  }
}


/* disk controller at '26 and '27

  NOTES:
  - in the DSEL disk channel program command, unit number is a 4-bit field,
  with these binary values:
  0001 = unit 0 (pdev 460/461)
  0010 = unit 1 (pdev 462/463)
  0100 = unit 2 (pdev 464/465)
  1000 = unit 3 (pdev 466/467)

  OCP '1626 = reset interrupt
  OCP '1726 = reset controller

  INA '1126 = input ID, don't clear A first, fails if no controller
  INA '1726 = read status (or something), fails if controller busy

  OTA '1726 = load OAR (Order Address Register), ie, run channel
  program, address is in A

 */

void devdisk (short class, short func, short device) {
  unsigned short oar;
  unsigned short status;       /* actual status */
  unsigned short teststatus;   /* status for order testing */
  unsigned short memaddr;
  unsigned short order;
  short halt;
  short unit;
  short head, track, rec, recsize, nwords;
  unsigned short dmachan, dmanch, dmaaddr, dmareg;
  short dmanw;
  char ordertext[8];
  char devfile[8];
  char devopened[8];        /* device file that is open on devfd */
  static int devfd=-1;      /* device file descriptor */
  int theads, spt, phyra;

  switch (class) {

  case 0:
    fprintf(stderr," OCP '%2o%2o\n", func, device);
    break;

  case 1:
    fprintf(stderr," SKS '%2o%2o\n", func, device);
    break;

  case 2:
    fprintf(stderr," INA '%2o%2o\n", func, device);
    if (func == 01)          /* read device id, clear A first */
      crs[A] = device;
    else if (func == 011)    /* read device id, don't clear A */
      crs[A] |= device;
    else if (func == 017)    /* read status */
      crs[A] = 0100000;
    else {
      fprintf(stderr," unimplemented INA device '%02o function\n", device);
      exit(1);
    }
    IOSKIP;
    break;

  case 3:
    fprintf(stderr," OTA '%02o%02o\n", func, device);
    if (func == 017) {        /* set OAR (order address register) */
      oar = crs[A];
      halt = 0;
      status = 0100000;
      unit = -1;
      while (!halt) {
	order = mem[oar]>>12;
	fprintf(stderr,"\n %o: %o %o %o\n", oar, mem[oar], mem[oar+1], mem[oar+2]);
	if (mem[oar] & 04000) {   /* "execute if ..." */
	  if (order == 2 || order == 5 || order == 6)
	    oar += 3;
	  else
	    oar += 2;
	  continue;
	}
	switch (order) {
	case 0: /* DHLT = Halt */
	  halt = 1;
	  fprintf(stderr," channel program halt at '%o\n", oar);
	  break;
	case 2: /* SFORM = Format */
	case 5: /* SREAD = Read */
	case 6: /* SWRITE = Write */
	  recsize = mem[oar] & 017;
	  track = mem[oar+1] & 01777;
	  rec = mem[oar+2] >> 8;   /* # records for format, rec # for R/W */
	  head = mem[oar+2] & 077;
	  if (order == 2)
	    strcpy(ordertext,"Format");
	  else if (order == 5)
	    strcpy(ordertext,"Read");
	  else if (order == 6)
	    strcpy(ordertext,"Write");
	  fprintf(stderr," %s, head=%d, track=%d, rec=%d, recsize=%d\n", ordertext, head, track, rec, recsize);
	  dmareg = ((dmachan & 036) << 1) | (dmachan & 1);
	  dmanw = regs.sym.regdmx[dmareg];
	  dmanw = -(dmanw>>4);
	  dmaaddr = regs.sym.regdmx[dmareg+1];
	  fprintf(stderr, " DMA channels: nch-1=%d, ['%o]='%o, ['%o]='%o, nwords=%d\n", dmanch, dmachan, regs.sym.regdmx[dmareg], dmachan+1, dmaaddr, dmanw);
	  if (devfd == -1) {
	    fprintf(stderr," Unit not selected or not ready\n");
	    status = 0100001;
	  } else if (order == 2)
	    fprintf(stderr," Format order not implemented\n");
	  else if (order == 5) {

	    /* translate head/track/sector to drive record address */

	    theads = 40;      /* should get total heads from a config file */
	    spt = 9;          /* and sectors per track too */
	    phyra = (track*theads*spt) + head*9 + rec;
	    fprintf(stderr, " phyra=%d, byte offset=%d\n", phyra, phyra*2080);
	    if (lseek(devfd, phyra*2080, SEEK_SET) == -1) {
	      perror("Unable to seek drive file");
	      exit(1);
	    }
	    if (read(devfd, mem+dmaaddr, dmanw*2) != dmanw*2) {
	      perror("Unable to read drive file");
	      exit(1);
	    }
	    regs.sym.regdmx[dmareg] = 0;
	    regs.sym.regdmx[dmareg+1] += dmanw;

	  } else if (order == 6)
	    fprintf(stderr," Write order not implemented\n");
	  oar += 3;
	  break;
	case 3: /* SSEEK = Seek */
	  track = mem[oar+1] & 01777;
	  fprintf(stderr," seek track %d, restore=%d, clear=%d\n", track, (mem[oar+1] & 0100000) != 0, (mem[oar+1] & 040000) != 0);
	  oar += 2;
	  break;
	case 4: /* DSEL = Select unit */
	  unit = (mem[oar+1] & 017) >> 1;  /* unit = 0/1/2/4 */
	  if (unit == 4) unit = 3;         /* unit = 0/1/2/3 */
	  snprintf(devfile,sizeof(devfile),"dev%ou%d", device, unit);
	  fprintf(stderr," select unit %d, filename %s\n", unit, devfile);
	  if (strcmp(devfile,devopened) != 0 || devfd == -1) {
	    if (devfd != -1) {
	      close(devfd);
	      devfd = -1;
	    }
	    if ((devfd = open(devfile, O_RDONLY, 0)) == -1)
	      status = 0100001;    /* not ready */
	    else
	      strcpy(devopened, devfile);
	  }
	  oar += 2;
	  break;
	case 7: /* DSTALL = Stall */
	  fprintf(stderr," stall\n");
	  oar += 2;
	  break;
	case 9: /* DSTAT = Store status to memory */
	  memaddr = mem[oar+1];
	  fprintf(stderr, " store status='%o to '%o\n", status, memaddr);
	  mem[memaddr] = status;
	  oar += 2;
	  break;
	case 11: /* DOAR = Store OAR to memory (2 words) */
	  memaddr = mem[oar+1];
	  fprintf(stderr, " store OAR='%o to '%o\n", oar, memaddr);
	  mem[memaddr] = oar;
	  oar += 2;
	  break;
	case 13: /* SDMA = select DMA channel(s) to use */
	  dmanch = mem[oar] & 017;
	  dmachan = mem[oar+1];
	  fprintf(stderr, " set DMA channels, nch-1=%d, channel='%o\n", dmanch, dmachan);
	  oar += 2;
	  break;
	case 14: /* DINT = generate interrupt through vector address */
	  memaddr = mem[oar+1];
	  fprintf(stderr, " interrupt through '%o\n", memaddr);
	  exit(1);
	  oar += 2;
	  break;
	case 15: /* DTRAN = channel program jump */
	  oar = mem[oar+1];
	  fprintf(stderr, " jump to '%o\n", oar);
	  break;
	default:
	  fprintf(stderr, " unrecognized channel order = %d\n", order);
	  exit(1);
	}
      }	  
      IOSKIP;
    } else {
      fprintf(stderr," unimplemented OTA device '%02o function\n", device);
      exit(1);
    }
    break;
  }
}
