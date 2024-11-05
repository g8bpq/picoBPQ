/*
Copyright 2001-2022 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
*/

//	Version 409p March 2005 Allow Multidigit COM Ports

//  Version 410h Jan 2009 Changes for Win98 Virtual COM
//		Open \\.\com instead of //./COM
//		Extra Dignostics

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#define WIN32

#ifndef WIN32

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

//#include <netax25/ttyutils.h>
//#include <netax25/daemon.h>

#include <netinet/tcp.h>

#ifdef MACBPQ
#define NOI2C
#endif

#ifdef FREEBSD
#define NOI2C
#endif

#ifdef NOI2C
int i2c_smbus_write_byte() {
  return -1;
}

int i2c_smbus_read_byte() {
  return -1;
}
#else
#include "i2c-dev.h"
#endif

//#define I2C_TIMEOUT	0x0702	/* set timeout - call with int 		*/

/* this is for i2c-dev.c	*/
//#define I2C_SLAVE	0x0703	/* Change slave address			*/
/* Attn.: Slave address is 7 or 10 bits */

#endif


#include "CHeaders.h"
#include "kiss.h"

int picoOpenSerial(char *Name, int speed);
int picoWriteSerial(int Chan, char *Name, int speed);
int picoReadSerial(int Chan, char *Name, int speed);
int picoCloseSerial(int Chan);

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD
#define QTSMKISSCMD 7

#define STX 2  // NETROM CONTROL CODES
#define ETX 3
#define DLE 0x10

#define CHECKSUM 1
#define POLLINGKISS 2  // KISSFLAGS BITS
#define ACKMODE 4      // CAN USE ACK REQURED FRAMES
#define POLLEDKISS 8   // OTHER END IS POLLING US
#define D700 16        // D700 Mode (Escape "C" chars
#define TNCX 32        // TNC-X Mode (Checksum of ACKMODE frames includes ACK bytes
#define PITNC 64       // PITNC Mode - can reset TNC with FEND 15 2
#define NOPARAMS 128   // Don't send SETPARAMS frame
#define FLDIGI 256     // Support FLDIGI COmmand Frames
#define TRACKER 512    // SCS Tracker. Need to set KISS Mode
#define FASTI2C 1024   // Use BLocked I2C Reads (like ARDOP)
#define DRATS 2048


int WritetoConsoleLocal(char *buff);
void INITCOMMON(struct KISSINFO *PORT);
struct PORTCONTROL *CHECKIOADDR(struct PORTCONTROL *OURPORT);
void INITCOM(struct KISSINFO *PORTVEC);
void SENDFRAME(struct KISSINFO *KISS, PMESSAGE Buffer);


void CloseKISSPort(struct PORTCONTROL *PortVector);
int ReadCOMBlockEx(int fd, char *Block, int MaxLength, int *Error);
void ConnecttoQtSM(struct PORTCONTROL *PORT);
void Debugprintf(const char *format, ...);
int OpenConnection(struct PORTCONTROL *PortVector);
void ProcessKISSFrame(struct PORTCONTROL * PORT, struct KISSINFO *KISS);

extern struct PORTCONTROL *PORTTABLE;
extern int NUMBEROFPORTS;
extern void *TRACE_Q;

#define TICKS 10  // Ticks per sec

// temp for testing

char lastblock[500];
int lastcount;

unsigned char ENCBUFF[600];

int ASYSEND(struct PORTCONTROL *PortVector, char *buffer, int count)
{
  return picoWriteSerial(PortVector->Handle, buffer, count);
}



int ASYINIT(int comport, int speed, struct PORTCONTROL *PortVector, char Channel)
{
  char Msg[80];
  struct KISSINFO *KISS = (struct KISSINFO *)PortVector;

  int BPQPort = PortVector->PORTNUMBER;

  if (PortVector->PROTOCOL == 2)  // NETROM
    sprintf(Msg, "ASYNC(NETROM) %s Chan %c ", PortVector->SerialPortName, Channel);
  else
    sprintf(Msg, "ASYNC %s Chan %c ", PortVector->SerialPortName, Channel);

  WritetoConsoleLocal(Msg);
 
  KISS->RXMSG = zalloc(512);
  KISS->RXMPTR = KISS->RXMSG;

  OpenConnection(PortVector);
  KISS->Portvector = PortVector;

  WritetoConsoleLocal("\n");

  return (0);
}


int OpenConnection(struct PORTCONTROL *PortVector)
{
  struct KISSINFO *KISS = (struct KISSINFO *)PortVector;
  int ComDev = 0;

  if (KISS == NULL)
    return 0;

  PortVector->Handle = picoOpenSerial(PortVector->SerialPortName, PortVector->BAUDRATE);

  if (PortVector->Handle == 0)
    return 0;

  if (KISS && KISS->KISSCMD && KISS->KISSCMDLEN)
    ASYSEND(PortVector, KISS->KISSCMD, KISS->KISSCMDLEN);


  return PortVector->Handle;
}

VOID KISSCLOSE(struct PORTCONTROL *PortVector) 
{
  CloseKISSPort(PortVector);
}

void CloseKISSPort(struct PORTCONTROL *PortVector) 
{
  // Just close the device - leave rest of info intact

  picoCloseSerial(PortVector->Handle);
  PortVector->Handle = 0;
}

static void CheckReceivedData(struct PORTCONTROL *PORT, PKISSINFO KISS)
{
  UCHAR c;
  int Len = 0;
  int i = 0;
  unsigned char Buffer[300];



  Len = picoReadSerial(PORT->Handle, Buffer, 256);

  if (Len == 0)
    return;

  while (Len--)
  {
   if (KISS->RXMPTR - KISS->RXMSG > 500)
     KISS->RXMPTR = KISS->RXMSG;

    c = Buffer[i++];

    if (PORT->PROTOCOL == 2)  // NETROM
    {
      if (KISS->NEEDCRC)  //Looking for CRC following ETX
      {
        ProcessKISSFrame(PORT, KISS);
        KISS->NEEDCRC = FALSE;
        *(KISS->RXMPTR++) = c;  // Save CRC
        continue;
      }

      if (KISS->ESCFLAG)
      {
        //
        //	DLE received - next should be DLE, STXor ETX, but we just pass on

        KISS->ESCFLAG = FALSE;
        *(KISS->RXMPTR++) = c;
        continue;
      } else {
        // see if DLE, if so set ESCFLAG and ignore

        if (c == DLE) {
          KISS->ESCFLAG = TRUE;
          continue;
        }
      }

      switch (c) {
        case STX:

          KISS->RXMPTR = (UCHAR *)&KISS->RXMSG;  // Reset buffer
          break;

        case ETX:
          KISS->NEEDCRC = TRUE;
          break;

        default:
          *(KISS->RXMPTR++) = c;
      }
      continue;
    }

    if (KISS->ESCFLAG)
    {
      //
      //	FESC received - next should be TFESC or TFEND

      KISS->ESCFLAG = FALSE;

      if (c == TFESC)
        c = FESC;

      if (c == TFEND)
        c = FEND;

    }
    else
    {
      switch (c)
      {
        case FEND:

          //
          //	Either start of message or message complete
          //

    

          if (KISS->RXMPTR == KISS->RXMSG)
          {
            // Start of Message. If polling, extend timeout

            if (PORT->KISSFLAGS & POLLINGKISS)
              KISS->POLLFLAG = 5 * TICKS;  // 5 SECS - SHOULD BE PLENTY

            continue;
          }

          ProcessKISSFrame(PORT, KISS);
          return;

        case FESC:

          KISS->ESCFLAG = TRUE;
          continue;
      }
    }

    //
    //	Ok, a normal char
    //

    *(KISS->RXMPTR++) = c;

    // if control byte, and equal to 0x0e, must set ready - poll responses dont have a trailing fend

    if (((c & 0x0f) == 0x0e) && (KISS->RXMPTR - KISS->RXMSG) == 1)
    {
      ProcessKISSFrame(PORT, KISS);
      continue;
    }
  }

  return;
}

// Code moved from KISSASM

unsigned short CRCTAB[256] = {
  0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
  0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
  0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
  0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
  0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
  0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
  0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
  0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
  0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
  0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
  0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
  0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
  0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
  0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
  0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
  0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
  0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
  0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
  0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
  0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
  0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
  0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
  0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
  0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
  0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
  0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
  0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
  0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
  0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
  0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
  0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
  0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};



void KISSINIT(struct KISSINFO *KISS) {
  PORTCONTROLX *PORT = (struct PORTCONTROL *)KISS;
  struct KISSINFO *FIRSTCHAN = NULL;

  if (PORT->CHANNELNUM == 0)
    PORT->CHANNELNUM = 'A';

  // As transition aid warn if Interlock is used on kiss port

  if (PORT->PORTINTERLOCK)
    WritetoConsoleLocal("Interlock defined on KISS port - is this intended? ");

  FIRSTCHAN = (struct KISSINFO *)CHECKIOADDR(PORT);  // IF ANOTHER ENTRY FOR THIS ADDR
                                                     // MAY BE CHANGED IN ANOTHER CHANNEL USED

  KISS->OURCTRL = (PORT->CHANNELNUM - 'A') << 4;  // KISS CONTROL

  if (FIRSTCHAN) {
    //	THIS IS NOT THE FIRST PORT ON THIS I/O ADDR - WE MUST BE USING
    //	AN ADDRESSABLE PROTOCOL - IE KISS AS DEFINED FOR KPC4 ETC

    KISS->FIRSTPORT = FIRSTCHAN;  // QUEUE TX FRAMES ON FIRST

    //	SET UP SUBCHANNEL CHAIN - ALL PORTS FOR THIS IO ADDR ARE CHAINED

    while (FIRSTCHAN->SUBCHAIN) {
      FIRSTCHAN = FIRSTCHAN->SUBCHAIN;
    }

    FIRSTCHAN->SUBCHAIN = KISS;  // PUT OURS ON END
    INITCOMMON(KISS);

    Consoleprintf("ASYNC %s Chan %c \n", PORT->SerialPortName, PORT->CHANNELNUM);
  } else
    INITCOM(KISS);

  //	Display to Console
}

void INITCOM(struct KISSINFO *KISS) {
  struct PORTCONTROL *PORT = (struct PORTCONTROL *)KISS;

  //	FIRST PORT USING THIS IO ADDRESS

  KISS->FIRSTPORT = KISS;
  KISS->POLLPOINTER = KISS;  // SET FIRST PORT TO POLL

  INITCOMMON(KISS);  // SET UP THE PORT

  //	ATTACH WIN32 ASYNC DRIVER

  ASYINIT(PORT->IOBASE, PORT->BAUDRATE, PORT, PORT->CHANNELNUM);

  return;
}


//struct PORTCONTROL * CHECKIOADDR(struct PORTCONTROL * OURPORT)
struct PORTCONTROL *CHECKIOADDR(struct PORTCONTROL *OURPORT) {
  //	SEE IF ANOTHER PORT IS ALREADY DEFINED ON THIS CARD

  struct PORTCONTROL *PORT = PORTTABLE;
  int n = NUMBEROFPORTS;

  while (n--) {
    if (PORT == OURPORT)  // NONE BEFORE OURS
      return NULL;        // None before us


    if (OURPORT->SerialPortName && strcmp(OURPORT->SerialPortName, "NOPORT") != 0) {
      // We are using a name

      if (PORT->SerialPortName && strcmp(PORT->SerialPortName, OURPORT->SerialPortName) == 0)
        return PORT;
    } else {
      // Using numbers not names

      if (PORT->IOBASE == OURPORT->IOBASE)
        return PORT;  // ANOTHER FOR SAME ADDRESS
    }

    PORT = PORT->PORTPOINTER;
  }

  return NULL;
}

void INITCOMMON(struct KISSINFO *KISS) {
  struct PORTCONTROL *PORT = (struct PORTCONTROL *)KISS;

  if (PORT->PROTOCOL == 2)  // NETROM?
  {
    PORT->KISSFLAGS = 0;  //	CLEAR KISS OPTIONS, JUST IN CASE!

    //	CMP	FULLDUPLEX[EBX],1	; NETROM DROPS RTS TO INHIBIT!
    //	JE SHORT NEEDRTS
    //
    //	MOV	AL,9			; OUT2 DTR
  }

  //	IF KISS, SET TIMER TO SEND KISS PARAMS

  if (PORT->PROTOCOL != 2)  // NETROM?
    if ((PORT->KISSFLAGS & NOPARAMS) == 0)
      PORT->PARAMTIMER = TICKS * 30;  //30 SECS FOR TESTING
}

void KISSTX(struct KISSINFO *KISS, PMESSAGE Buffer) {
  struct PORTCONTROL *PORT = (struct PORTCONTROL *)KISS;

  //	START TRANSMISSION


  KISS = KISS->FIRSTPORT;  // ALL FRAMES GO ON SAME Q

  if ((PORT->KISSFLAGS & POLLEDKISS) || KISS->KISSTX_Q || KISS->FIRSTPORT->POLLFLAG || KISS->TXACTIVE) {
    // POLLED or ALREADY SOMETHING QUEUED or POLL OUTSTANDING - MUST QUEUE

    C_Q_ADD(&KISS->KISSTX_Q, Buffer);
    return;
  }

  /*
;	IF NETROM PROTOCOL AND NOT FULL DUPLEX AND BUSY, QUEUE IT
;
	CMP	PROTOCOL[EBX],2		; NETROM?
	JNE SHORT DONTCHECKDCD		; NO

	CMP	FULLDUPLEX[EBX],1
	JE SHORT DONTCHECKDCD		; FULLDUP - NO CHECK
;
;	NETROM USES RTS, CROSS-CONNECTED TO CTS AT OTHER END, TO MEAN
;	NOT BUSY
;
;	MOV	DX,MSR[EBX]
;	IN	AL,DX
;
;	TEST	AL,CTSBIT		; CTS HIGH?
;	JZ SHORT QUEUEIT			; NO, SO QUEUE FRAME
;
;	GOING TO SEND - DROP RTS TO INTERLOCK OTHERS 
;
;	MOV	DX,MCR[EBX]
;	MOV	AL,09H			; DTR OUT2
;
;	OUT	DX,AL
;
;	MAKE SURE CTS IS STILL ACTIVE - IF NOT, WE HAVE A COLLISION,
;	SO RELEASE RTS AND WAIT
;
;	DELAY

;	MOV	DX,MSR[EBX]
;	IN	AL,DX
;	TEST	AL,CTSBIT
;	JNZ SHORT DONTCHECKDCD		; STILL HIGH, SO CAN SEND
;
;	RAISE RTS AGAIN, AND QUEUE FRAME
;
;	DELAY

;	MOV	DX,MCR[EBX]
;	MOV	AL,0BH			; RTS DTR OUT2
;
;	OUT	DX,AL
;
	JMP	QUEUEIT

DONTCHECKDCD:
*/

  SENDFRAME(KISS, Buffer);
}

void SENDFRAME(struct KISSINFO *KISS, PMESSAGE Buffer)
{
  PPORTCONTROL PORT = (struct PORTCONTROL *)KISS;
  struct _MESSAGE *Message = (struct _MESSAGE *)Buffer;
  UCHAR c;

  int Portno;
  char *ptr1, *ptr2;
  int Len;

  //	GET REAL PORT TABLE ENTRY - IF MULTIPORT, FRAME IS QUEUED ON FIRST

  if (PORT->PROTOCOL == 2)  // NETROM
  {
    UCHAR TXCCC = 0;

    ptr1 = &Message->DEST[0];
    Len = Message->LENGTH - MSGHDDRLEN;
    ENCBUFF[0] = STX;
    ptr2 = &ENCBUFF[1];

    while (Len--) {
      c = *(ptr1++);
      TXCCC += c;

      switch (c) {
        case DLE:
          (*ptr2++) = DLE;
          (*ptr2++) = DLE;
          break;

        case STX:
          (*ptr2++) = DLE;
          (*ptr2++) = STX;
          break;

        case ETX:
          (*ptr2++) = DLE;
          (*ptr2++) = ETX;
          break;

        default:
          (*ptr2++) = c;
      }
    }

    (*ptr2++) = ETX;  // NETROM has CRC after ETX
    (*ptr2++) = TXCCC;

    ASYSEND(PORT, ENCBUFF, (int)(ptr2 - (char *)ENCBUFF));
    //		Debugprintf("NETROM TX Len %d CRC %d", ptr2 - (char *)ENCBUFF, TXCCC);

    C_Q_ADD(&TRACE_Q, Buffer);
    return;
  }

  Portno = Message->PORT;

  while (KISS->PORT.PORTNUMBER != Portno) {
    KISS = KISS->SUBCHAIN;

    if (KISS == NULL) {
      ReleaseBuffer(Buffer);
      return;
    }
  }

  //	Encode frame

  ptr1 = &Message->DEST[0];
  Len = Message->LENGTH - (3 + sizeof(void *));
  ENCBUFF[0] = FEND;
  ENCBUFF[1] = KISS->OURCTRL;
  ptr2 = &ENCBUFF[2];

  KISS->TXCCC = 0;

  //	See if ACKMODE needed

  // Make sure we look on correct port if a subport

  if (KISS->PORT.KISSFLAGS & ACKMODE) {
    if (Buffer->Linkptr)  // Frame Needs ACK
    {
      UINT ACKWORD = (UINT)(Buffer->Linkptr - LINKS);
      ENCBUFF[1] |= 0x0c;  // ACK OPCODE
      ENCBUFF[2] = ACKWORD & 0xff;
      ENCBUFF[3] = (ACKWORD >> 8) & 0xff;

      // have to reset flag so trace doesnt clear it

      Buffer->Linkptr = 0;

      if (KISS->PORT.KISSFLAGS & TNCX) {
        // Include ACK bytes in Checksum

        KISS->TXCCC ^= ENCBUFF[2];
        KISS->TXCCC ^= ENCBUFF[3];
      }
      ptr2 = &ENCBUFF[4];
    }
  }

  KISS->TXCCC ^= ENCBUFF[1];

  while (Len--) {
    c = *(ptr1++);
    KISS->TXCCC ^= c;

    switch (c) {
      case FEND:
        (*ptr2++) = FESC;
        (*ptr2++) = TFEND;
        break;

      case FESC:

        (*ptr2++) = FESC;
        (*ptr2++) = TFESC;
        break;

      case 'C':

        if (KISS->PORT.KISSFLAGS & D700) {
          (*ptr2++) = FESC;
          (*ptr2++) = 'C';
          break;
        }

        // Drop through

      default:

        (*ptr2++) = c;
    }
  }

  // If using checksum, send it

  if (KISS->PORT.KISSFLAGS & CHECKSUM) {
    c = (UCHAR)KISS->TXCCC;

    // On TNC-X based boards, it is difficult to cope with an encoded CRC, so if
    // CRC is FEND, send it as 0xc1. This means we have to accept 00 or 01 as valid.
    // which is a slight loss in robustness

    if (c == FEND && (PORT->KISSFLAGS & TNCX)) {
      (*ptr2++) = FEND + 1;
    } else {
      switch (c) {
        case FEND:
          (*ptr2++) = FESC;
          (*ptr2++) = TFEND;
          break;

        case FESC:
          (*ptr2++) = FESC;
          (*ptr2++) = TFESC;
          break;

        default:
          (*ptr2++) = c;
      }
    }
  }

  (*ptr2++) = FEND;

  ASYSEND(PORT, ENCBUFF, (int)(ptr2 - (char *)ENCBUFF));

  // Pass buffer to trace routines

  C_Q_ADD(&TRACE_Q, Buffer);
}


void KISSTIMER(struct KISSINFO *KISS) {
  struct PORTCONTROL *PORT = (struct PORTCONTROL *)KISS;
  PMESSAGE Buffer;

  //	SEE IF TIME TO REFRESH KISS PARAMS

  if (((PORT->KISSFLAGS & (POLLEDKISS | NOPARAMS)) == 0) && PORT->PROTOCOL != 2) {
    PORT->PARAMTIMER--;

    if (PORT->PARAMTIMER == 0) {
      //	QUEUE A 'SET PARAMS' FRAME

      if (PORT->PORTDISABLED == 0) {
        unsigned char *ptr = ENCBUFF;

        *(ptr++) = FEND;
        *(ptr++) = KISS->OURCTRL | 1;
        *(ptr++) = (UCHAR)PORT->PORTTXDELAY;
        *(ptr++) = FEND;

        *(ptr++) = FEND;
        *(ptr++) = KISS->OURCTRL | 2;
        *(ptr++) = PORT->PORTPERSISTANCE;
        *(ptr++) = FEND;

        *(ptr++) = FEND;
        *(ptr++) = KISS->OURCTRL | 3;
        *(ptr++) = PORT->PORTSLOTTIME;
        *(ptr++) = FEND;

        *(ptr++) = FEND;
        *(ptr++) = KISS->OURCTRL | 4;
        *(ptr++) = PORT->PORTTAILTIME;
        *(ptr++) = FEND;

        *(ptr++) = FEND;
        *(ptr++) = KISS->OURCTRL | 5;
        *(ptr++) = PORT->FULLDUPLEX;
        *(ptr++) = FEND;

        PORT = (struct PORTCONTROL *)KISS->FIRSTPORT;  // ALL FRAMES GO ON SAME Q

        ASYSEND(PORT, ENCBUFF, (int)(ptr - &ENCBUFF[0]));
      }
      KISS->PORT.PARAMTIMER = TICKS * 60 * 5;  // 5 MINS
    }
  }

  //	IF FRAMES QUEUED, AND NOT SENDING, START

  if (KISS == KISS->FIRSTPORT)  // ALL FRAMES GO ON SAME Q
  {
    //	SEE IF POLL HAS TIMED OUT

    if (PORT->KISSFLAGS & POLLINGKISS) {
      if (KISS->POLLFLAG)  // TIMING OUT OR RECEIVING
      {
        KISS->POLLFLAG--;

        if (KISS->POLLFLAG == 0) {
          //	POLL HAS TIMED OUT - MAY NEED TO DO SOMETHING

          KISS->POLLPOINTER->PORT.L2URUNC++;  // PUT IN UNDERRUNS FIELD
        }
      }
    }
  }
  /*
;
;	WAITING FOR CTS
;
;	MOV	DX,MSR[EBX]
;	IN	AL,DX
;	TEST	AL,CTSBIT
;	JNZ SHORT TIMERSEND		; OK TO SEND NOW
;
*/


  //	SEE IF ANYTHING TO SEND

  if ((PORT->KISSFLAGS & POLLEDKISS) == 0 || KISS->POLLED) {
    // OK to Send

    if (KISS->KISSTX_Q) {
      //	IF NETROM MODE AND NOT FULL DUP, CHECK DCD

      KISS->POLLED = 0;

      //CMP	PROTOCOL[EBX],2		; NETROM?
      //JNE SHORT DONTCHECKDCD_1

      //CMP	FULLDUPLEX[EBX],1
      //JE SHORT DONTCHECKDCD_1
      //TEST	AL,CTSBIT		; DCD HIGH?
      //	JZ SHORT NOTHINGTOSEND		; NO, SO WAIT

      //	DROP RTS TO LOCK OUT OTHERS

      //	MOV	DX,MCR[EBX]
      //	MOV	AL,09H			; DTR OUT2

      //	OUT	DX,AL


      //	MAKE SURE CTS IS STILL ACTIVE - IF NOT, WE HAVE A COLLISION,
      //	SO RELEASE RTS AND WAIT

      //	DELAY

      //	MOV	DX,MSR[EBX]
      //	IN	AL,DX
      //	TEST	AL,CTSBIT
      //	JNZ SHORT TIMERSEND		; STILL HIGH, SO CAN SEND

      //	RAISE RTS AGAIN, AND WAIT A BIT MORE

      //	DELAY

      //MOV	DX,MCR[EBX]
      //	MOV	AL,0BH			; RTS DTR OUT2

      //	OUT	DX,AL


      Buffer = Q_REM(&KISS->KISSTX_Q);
      SENDFRAME(KISS, Buffer);
      return;
    }
  }

  // Nothing to send. IF POLLED MODE, SEND A POLL TO NEXT PORT

  if ((PORT->KISSFLAGS & POLLINGKISS) && KISS->FIRSTPORT->POLLFLAG == 0) {
    struct KISSINFO *POLLTHISONE;

    KISS = KISS->FIRSTPORT;  // POLLPOINTER is in first port

    //	FIND WHICH CHANNEL TO POLL NEXT

    POLLTHISONE = KISS->POLLPOINTER->SUBCHAIN;  // Next to poll

    if (POLLTHISONE == NULL)
      POLLTHISONE = KISS;  // Back to start

    KISS->POLLPOINTER = POLLTHISONE;  // FOR NEXT TIME

    KISS->POLLFLAG = TICKS / 2;  // ALLOW 1/3 SEC

    ENCBUFF[0] = FEND;
    ENCBUFF[1] = POLLTHISONE->OURCTRL | 0x0e;  // Poll
    ENCBUFF[2] = FEND;

    ASYSEND((struct PORTCONTROL *)KISS, ENCBUFF, 3);
  }

  return;
}

int KISSRX(struct KISSINFO *KISS) 
{
  struct PORTCONTROL *PORT = (struct PORTCONTROL *)KISS;
  PMESSAGE Buffer;
  int len;


  if (KISS == 0)
    return 0;  // Just in case

  CheckReceivedData(PORT, KISS);  // Look for data in RXBUFFER and COM port

  return 0;
}

void ProcessKISSFrame(struct PORTCONTROL * PORT, struct KISSINFO *KISS)
{
  PMESSAGE Buffer;
  int len;

  len = (int)(KISS->RXMPTR - KISS->RXMSG);

  // reset pointers

  KISS->RXMPTR = (UCHAR *)KISS->RXMSG;

  if (len > 329)  // Max ax.25 frame + KISS Ctrl
  {
    if (PORT)
      Debugprintf("BPQ32 overlong KISS frame - len = %d Port %d", len, PORT->PORTNUMBER);
    return;
  }


  //	IF NETROM, CAN PASS ON NOW

  if (PORT->PROTOCOL == 2)
  {
    // But need to checksum.

    UCHAR c = 0;
    int i;
    UCHAR *ptr = KISS->RXMSG;

    len--;  // Included CRC

    for (i = 0; i < len; i++)
      c += *(ptr++);

    //		Debugprintf("NETROM RX Len %d CRC %d", len, *ptr);


    if (c != *ptr)  // CRC OK?
    {
      PORT->RXERRORS++;
      Debugprintf("NETROM Checksum Error %d %d", c, *ptr);
      return;
    }

    Buffer = GetBuff();

    if (Buffer) {
      memcpy(&Buffer->DEST, KISS->RXMSG, len);
      len += (3 + sizeof(void *));

      PutLengthinBuffer((PDATAMESSAGE)Buffer, len);

      C_Q_ADD(&PORT->PORTRX_Q, (UINT *)Buffer);
    }

    return;
  }

  //	Any response should clear POLL OUTSTANDING

  KISS->POLLFLAG = 0;  // CLEAR POLL OUTSTANDING

  // See if message is a poll (or poll response)

  if ((KISS->RXMSG[0] & 0x0f) == 0x0e)  // POLL Frame
  {
    int PolledPort;

    if (PORT->KISSFLAGS & POLLINGKISS) {
      // Make Sure response is from the device I polled

      if ((KISS->RXMSG[0] & 0xf0) == KISS->POLLPOINTER->OURCTRL) {
        // if Nothing queued for tx, poll again (to speed up response)

        if (KISS->KISSTX_Q == 0) {
          struct KISSINFO *POLLTHISONE;

          //	FIND WHICH CHANNEL TO POLL NEXT

          POLLTHISONE = KISS->POLLPOINTER->SUBCHAIN;  // Next to poll

          if (POLLTHISONE == NULL)
            POLLTHISONE = KISS;  // Back to start

          KISS->POLLPOINTER = POLLTHISONE;  // FOR NEXT TIME

          KISS->POLLFLAG = TICKS / 2;  // ALLOW 1/3 SEC

          ENCBUFF[0] = FEND;
          ENCBUFF[1] = POLLTHISONE->OURCTRL | 0x0e;  // Poll
          ENCBUFF[2] = FEND;

          ASYSEND((struct PORTCONTROL *)KISS, ENCBUFF, 3);
        }
      } else
        Debugprintf("Polled KISS - response from wrong address - Polled %d Reponse %d",
                    KISS->POLLPOINTER->OURCTRL, (KISS->RXMSG[0] & 0xf0));

      return;
    }

    //	WE ARE A SLAVE, AND THIS IS A POLL. SEE IF FOR US, AND IF SO, REPLY

    PolledPort = KISS->RXMSG[0] & 0xf0;

    while (KISS->OURCTRL != PolledPort) {
      KISS = KISS->SUBCHAIN;
      if (KISS == NULL)
         return;
    }

    //	SEE IF ANYTHING QUEUED

    if (KISS->KISSTX_Q) {
      KISS->POLLED = 1;  // LET TIMER DO THE SEND
      return;
    }

    ENCBUFF[0] = FEND;
    ENCBUFF[1] = KISS->OURCTRL | 0x0e;  // Poll/Poll Resp
    ENCBUFF[2] = FEND;

    ASYSEND(PORT, ENCBUFF, 3);
     return;
  }

  //	MESSAGE MAY BE DATA OR DATA ACK. IT HAS NOT YET BEEN CHECKSUMMED

  if ((KISS->RXMSG[0] & 0x0f) == 0x0c)  // ACK Frame
  {
    //	ACK FRAME. WE DONT SUPPORT ACK REQUIRED FRAMES AS A SLAVE - THEY ARE ONLY ACCEPTED BY TNCS

    struct _LINKTABLE *LINK;
    int ACKWORD = KISS->RXMSG[1] | KISS->RXMSG[2] << 8;

    if (ACKWORD < MAXLINKS) {
      LINK = LINKS + ACKWORD;

      if (LINK->L2TIMER)
        LINK->L2TIMER = LINK->L2TIME;
    }
    return;
  }

  if (KISS->RXMSG[0] & 0x0f)  // Not Data
  {
    // See if QTSM Status Packet

    if ((KISS->RXMSG[0] & 0x0f) == QTSMKISSCMD) 
    {
      unsigned char *Msg = &KISS->RXMSG[1];
      int Chan = KISS->RXMSG[0] & 0xf0;
      len--;

      Msg[len] = 0;

      while (KISS->OURCTRL != Chan) 
      {
        KISS = KISS->SUBCHAIN;

        if (KISS == NULL)
           return;
      }

      //	ok, KISS now points to our port

      Debugprintf("%d %x %s", PORT->PORTNUMBER, KISS->RXMSG[0], Msg);

      if (memcmp(Msg, "STATS ", 6) == 0) {
        // Save busy

        int TX, DCD;
        char *Msg1 = strlop(&Msg[6], ' ');

        TX = atoi(&Msg[6]);
        if (Msg1) {
          DCD = atoi(Msg1);

          KISS->PORT.AVSENDING = TX;
          KISS->PORT.AVACTIVE = DCD + TX;

          KISS->QtSMStats = 1;
        }
      } else if (memcmp(Msg, "PKTINFO ", 8) == 0) {
        // Save State

        char *Msg1 = &Msg[8];

        if (strlen(Msg) > 63)
          Msg[63] = 0;

        strcpy(PORT->PktFlags, Msg1);
      } else
        Debugprintf("Unknown Command %d %x %s", PORT->PORTNUMBER, KISS->RXMSG[0], Msg);

      // Note status applies to NEXT data packet received
    }


    // If a reply to a manual KISS command(Session set and time not too long ago)
    // send reponse to terminal

    if (PORT->Session && (time(NULL) - PORT->LastKISSCmdTime < 10)) {
      PDATAMESSAGE Buffer;
      BPQVECSTRUC *VEC;
      unsigned char *Msg = &KISS->RXMSG[1];
      len--;

      Msg[len] = 0;

      Buffer = GetBuff();
      if (Buffer) {
        int i;
        unsigned char c;
        int hex = 0;

        // Could be text or hex response

        for (i = 0; i < len; i++) {
          c = Msg[i];

          if (c != 10 && c != 13 && (c < 31 || c > 127)) {
            hex = 1;
            break;
          }
        }

        Buffer->PID = 0xf0;
        Buffer->LENGTH = MSGHDDRLEN + 1;  // Includes PID

        if (hex == 0)
          Buffer->LENGTH += sprintf(Buffer->L2DATA, "%s\r", Msg);
        else {
          if (len == 80)
            len = 80;  // Protect buffer

          for (i = 0; i < len; i++) {
            Buffer->LENGTH += sprintf(&Buffer->L2DATA[i * 3], "%02X ", Msg[i]);
          }
          Buffer->LENGTH += sprintf(&Buffer->L2DATA[i * 3], "\r");
        }

        VEC = PORT->Session->L4TARGET.HOST;
        C_Q_ADD(&PORT->Session->L4TX_Q, (UINT *)Buffer);
      }
      PORT->Session = 0;
    }

    KISS->RXMSG[len] = 0;

    //		Debugprintf(KISS->RXMSG);
    return;
  }

  //	checksum if necessary


  if (len < 15)
    return;  // too short for AX25

  if (PORT->KISSFLAGS & CHECKSUM) {
    //	SUM MESSAGE, AND IF DUFF DISCARD. IF OK DECREMENT COUNT TO REMOVE SUM

    int sumlen = len;
    char *ptr = &KISS->RXMSG[0];
    UCHAR sum = 0;

    while (sumlen--) {
      sum ^= *(ptr++);
    }

    if (sum) {
      PORT->RXERRORS++;
      Debugprintf("KISS Checksum Error");

      PORT->PktFlags[0] = 0;
      return;
    }
    len--;  // Remove Checksum
  }

  //	FIND CORRECT SUBPORT RECORD

  while (KISS->OURCTRL != KISS->RXMSG[0])
  {
    KISS = KISS->SUBCHAIN;

    if (KISS == NULL) {
      // Unknown channel - clear any status info

      PORT->PktFlags[0] = 0;
      Debugprintf("bad channel");
     return;
    }
  }

  //	ok, KISS now points to our port

  Buffer = GetBuff();

  // we dont need the control byte

  len--;

  if (Buffer) {
    memcpy(&Buffer->DEST, &KISS->RXMSG[1], len);
    len += (3 + sizeof(void *));

    PutLengthinBuffer((PDATAMESSAGE)Buffer, len);  // Needed for arm5 portability

    if (PORT->PktFlags[0])
      strcpy(Buffer->Padding, PORT->PktFlags);
    else
      Buffer->Padding[0] = 0;

    PORT->PktFlags[0] = 0;

    /*
		// Randomly drop packets

		if ((rand() % 7) > 5)
		{
			Debugprintf("KISS Test Drop packet");
			ReleaseBuffer(Buffer);
		}
		else
*/
    C_Q_ADD(&KISS->PORT.PORTRX_Q, (UINT *)Buffer);
  }

  return;
}


void ConnecttoQtSM(struct PORTCONTROL *PORT) {
}
