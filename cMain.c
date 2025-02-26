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


//
//	C replacement for Main.asm
//
#define Kernel

#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

//#include "windows.h"
//#include "winerror.h"

#include "time.h"
#include "stdio.h"
#include <fcntl.h>					 


#include "CHeaders.h"


VOID L2Routine(struct PORTCONTROL * PORT, PMESSAGE Buffer);
VOID ProcessIframe(struct _LINKTABLE * LINK, PDATAMESSAGE Buffer);
VOID FindLostBuffers();
VOID ReadMH();
void GetPortCTEXT(TRANSPORTENTRY * Session, char * Bufferptr, char * CmdTail, struct CMDX * CMD);
int upnpInit();
void AISTimer();
void ADSBTimer();
VOID SendSmartID(struct PORTCONTROL * PORT);
int CanPortDigi(int Port);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
void MQTTTimer();
int getFreeHeap();


void * picoOpenFile(char * FN);
int picoGetLine(void * file, char * Line, int maxline);
int GetNextFileBlock(void * file, char * line);
void picoCloseFile(void * file);
void picoWriteLine(void * file, char * Data, int len);
void * picoCreateFile(char * FN);
void picofprintf(void * file, const char * format, ...);



#include "configstructs.h"

extern struct CONFIGTABLE * xxcfg;
extern BOOL needAIS;
extern int needADSB;
extern char PWTEXT[];
extern int PWLen;

extern struct CMDX COMMANDS[200];

struct PORTCONFIG * PortRec;

#define RNRSET 0x2				// RNR RECEIVED FROM OTHER END

//	STATION INFORMATION

char DATABASESTART[14] = "";
char xMAJORVERSION = 4;
char xMINORVERSION = 9;
char FILLER1[16] = "";

struct ROUTE * NEIGHBOURS = NULL;
int  ROUTE_LEN = sizeof(struct ROUTE);
int  MAXNEIGHBOURS = 20;

struct DEST_LIST * DESTS = NULL;		// NODE LIST
int  DEST_LIST_LEN = sizeof(struct DEST_LIST);

struct _LINKTABLE * LINKS = NULL;
int	LINK_TABLE_LEN = sizeof (struct _LINKTABLE); 
int	MAXLINKS = 30;


char	MYCALL[7] = ""; //		DB	7 DUP (0)	; NODE CALLSIGN (BIT SHIFTED)
char	MYALIASTEXT[6] = ""; //	DB	'      '	; NODE ALIAS (KEEP TOGETHER)

char MYALIASLOPPED[10];
char MYCALLLOPPED[10];

UCHAR	MYCALLWITHALIAS[13] = "";

UCHAR	NETROMCALL[7] = "";				// Call used for NETROM (can be MYCALL)

APPLCALLS APPLCALLTABLE[NumberofAppls] = {0};

UCHAR	MYNODECALL[10] = "";				// NODE CALLSIGN (ASCII)
UCHAR	MYNETROMCALL[10] = "";				// NETROM CALLSIGN (ASCII)
char NODECALLLOPPED[10];

VOID * FREE_Q = NULL;

time_t TimeLoaded = 0;

struct PORTCONTROL * PORTTABLE = NULL;
int	NUMBEROFPORTS = 0;
int PORTENTRYLEN = sizeof(struct PORTCONTROL);

struct DEST_LIST * ENDDESTLIST = NULL;		//		; NODE LIST+1


unsigned char * WorkBuffer;


int OBSINIT = 5;				// INITIAL OBSOLESCENCE VALUE
int OBSMIN = 4;					// MINIMUM TO BROADCAST
int L3INTERVAL = 60;			// 'NODES' INTERVAL IN MINS
int IDINTERVAL = 20;			// 'ID' BROADCAST INTERVAL
int BTINTERVAL = 20;			// 'BT' BROADCAST INTERVAL
int MINQUAL = 10;				// MIN QUALITY FOR AUTOUPDATES
int HIDENODES = 0;				// N * COMMAND SWITCH
int BBSQUAL = 255;				// QUALITY OF BBS RELATIVE TO NODE

int NUMBEROFBUFFERS = 100;		// PACKET BUFFERS

int PACLEN = 100;				//MAX PACKET SIZE

//	L2 SYSTEM TIMER RUNS AT 3 HZ

int T3 = 3*61*3;				// LINK VALIDATION TIMER (3 MINS) (+ a bit to reduce RR collisions)

int L2KILLTIME = 16*60*3;		// IDLE LINK TIMER (16 MINS)	
int L3LIVES = 15;				// MAX L3 HOPS
int L4N2 =  3;					// LEVEL 4 RETRY COUNT
int L4LIMIT = 60*15;			// IDLE SESSION LIMIT - 15 MINS
int L4DELAY = 5;				// L4 DELAYED ACK TIMER
	
int BBS = 1;					// INCLUDE BBS SUPPORT
int NODE = 1;					// INCLUDE SWITCH SUPPORT

int FULL_CTEXT = 1;				// CTEXT ON ALL CONNECTS IF NZ

int L4Compress = 0;
int L4CompMaxframe = 3;
int L4CompPaclen = 236;

BOOL LogL4Connects = FALSE;
BOOL LogAllConnects = FALSE;
BOOL AUTOSAVEMH = TRUE;
extern BOOL ADIFLogEnabled;
extern UCHAR LogDirectory[260];
extern BOOL EventsEnabled;
BOOL M0LTEMap = FALSE;
BOOL MQTT = FALSE;
char MQTT_HOST[80] = "";
int MQTT_PORT = 0;
char MQTT_USER[80] = "";
char MQTT_PASS[80] = "";

int MQTT_Connecting = 0;
int MQTT_Connected = 0;


//TNCTABLE	DD	0
//NUMBEROFSTREAMS	DD	0

extern VOID * ENDPOOL;
extern void * APPL_Q;				// Queue of frames for APRS Appl

extern BOOL APRSActive;

#define BPQHOSTSTREAMS	64

// Although externally streams are numbered 1 to 64, internally offsets are 0 - 63

BPQVECSTRUC XDUMMY = {0};					// Needed to force correct order of following

BPQVECSTRUC BPQHOSTVECTOR[BPQHOSTSTREAMS + 5] = {0};

BPQVECSTRUC * TELNETMONVECPTR = &BPQHOSTVECTOR[BPQHOSTSTREAMS];
BPQVECSTRUC * AGWMONVECPTR = &BPQHOSTVECTOR[BPQHOSTSTREAMS + 1];
BPQVECSTRUC * APRSMONVECPTR = &BPQHOSTVECTOR[BPQHOSTSTREAMS + 2];
BPQVECSTRUC * IPHOSTVECTORPTR = &BPQHOSTVECTOR[BPQHOSTSTREAMS + 3];

int BPQVECLENGTH = sizeof(BPQVECSTRUC);

int NODEORDER = 0;
UCHAR LINKEDFLAG = 0;

UCHAR UNPROTOCALL[80] = "";

UCHAR ExcludeList[71] = "";		// 10 ENTRIES, 7 BYTES EACH

char * INFOMSG = NULL;

char * CTEXTMSG = NULL;
int CTEXTLEN = 0;

UCHAR MYALIAS[7] = "";				// ALIAS IN AX25 FORM
UCHAR BBSALIAS[7] = "";

UCHAR AX25CALL[7] = "";				// WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
UCHAR NORMCALL[10] = "";			// CALLSIGN IN NORMAL FORMAT
int NORMLEN	= 0;					// LENGTH OF CALL IN NORMCALL	

int CURRENTPORT = 0;				// PORT FOR CURRENT MESSAGE
VOID * CURRENTPORTPTR = NULL;		// PORT CONTROL TABLE ENTRY FOR CURRENT PORT

int SDCBYTE = 0;					// CONTROL BYTE FOR CURRENT FRAME

VOID * BUFFER = NULL;				// GENERAL SAVE AREA FOR BUFFER ADDR
VOID * ADJBUFFER = NULL;			// BASE ADJUSED FOR DIGIS

UCHAR TEMPFIELD[7] = "";			// ADDRESS WORK FILED

void * TRACE_Q	= NULL;				// TRANSMITTED FRAMES TO BE TRACED

int RANDOM = 0;						// 'RANDOM' NUMBER FOR PERSISTENCE CALCS
 
int L2TIMERFLAG = 0;				// INCREMENTED AT 18HZ BY TIMER INTERRUPT
int L3TIMERFLAG = 0;				// DITTO
int L4TIMERFLAG = 0;				// DITTO

char HEADERCHAR	= '}';				// CHAR FOR _NODE HEADER MSGS

VOID * LASTPOINTER = NULL;			// PERVIOUS _NODE DURING CHAINING

int REALTIMETICKS = 0;
int BGTIMER = 0;					// TO CONTROL BG SCANS

VOID * CONFIGPTR = NULL;			// Internal Config Get Offset

int AUTOSAVE = 0;					// AUTO SAVE NODES ON EXIT FLAG
int L4APPL = 1;						// Application for BBSCALL/ALIAS connects
int CFLAG = 0;						// C =HOST Command

VOID * IDMSG_Q = NULL;				// ID/BEACONS WAITING TO BE SENT

int	NODESINPROGRESS = 0;
VOID * CURRENTNODE = NULL;			// NEXT _NODE TO SEND
VOID * DESTHEADER = NULL;			// HEAD OF SORTED NODES CHAIN

int	L3TIMER	= 1;					// TIMER FOR 'NODES' MESSAGE
int	IDTIMER = 0;					// TIMER FOR ID MESSAGE
int	BTTIMER = 0;					// TIMER FOR BT MESSAGE

int NEEDMH = 0;

struct DATAMESSAGE BTHDDR = {0,0,9,240,13};
struct _MESSAGE IDHDDR = {0,0,23,0,0,3, 240};

VOID * IDMSG = &IDHDDR;

//DD	0		; CHAIN
//			DB	0		; PORT	
int BTLENGTH = 9; //	DW	9		; LENGTH
//			DB	0F0H		; PID
char BTEXTFLD[256] ="\r";

char BridgeMap[MaxBPQPortNo + 1][MaxBPQPortNo + 1] = {0};

// Keep Buffers at end
	

void ** Bufferlist[250] = {0};

extern BOOL IPRequired;
extern BOOL PMRequired;
extern int MaxHops;
extern int MAXRTT;
extern USHORT CWTABLE[];
extern struct _TRANSPORTENTRY * L4TABLE;
extern UCHAR ROUTEQUAL;
extern UINT BPQMsg;

extern int NUMBEROFTNCPORTS;

extern APPLCALLS APPLCALLTABLE[];

//	LOOPBACK PORT ROUTINES

VOID LINKINIT(PEXTPORTDATA PORTVEC)
{
	WritetoConsoleLocal("Loopback\n");
}

VOID LINKTX(PEXTPORTDATA PORTVEC, PMESSAGE Buffer)
{
	//	LOOP BACK TO SWITCH
	struct _LINKTABLE * LINK;
	
	LINK = Buffer->Linkptr;

	if (LINK)
	{
		if (LINK->L2TIMER)
			LINK->L2TIMER = LINK->L2TIME;

		Buffer->Linkptr = 0;	// CLEAR FLAG FROM BUFFER
	}

	C_Q_ADD(&PORTVEC->PORTCONTROL.PORTRX_Q, Buffer);
}


VOID LINKRX()
{
}


VOID LINKTIMER()
{
}
	
VOID LINKCLOSE()
{
}


VOID EXTCLOSE()
{
}
	
BOOL KISSTXCHECK()
{
	return 0;
}

BOOL LINKTXCHECK()
{
	return 0;
}

void * Dummy()				// Dummy for missing EXT Driver
{
	return 0;
}


VOID PostDataAvailable(TRANSPORTENTRY * Session)
{

}

VOID PostStateChange(TRANSPORTENTRY * Session)
{

}

#ifdef LINBPQ

#define HDLCTX KHDLCTX
#define HDLCRX KHDLCRX
#define HDLCTIMER KHDLCTIMER
#define HDLCCLOSE KHDLCCLOSE
#define HDLCTXCHECK KHDLCTXCHECK

#define PC120INIT KHDLCINIT
#define DRSIINIT KHDLCINIT
#define TOSHINIT KHDLCINIT
#define RLC100INIT KHDLCINIT
#define BAYCOMINIT KHDLCINIT
#define PA0INIT KHDLCINIT

int KHDLCINIT(PHDLCDATA PORTVEC);
void KHDLCTX(struct KISSINFO * KISS, PMESSAGE Buffer);
int KHDLCRX(PHDLCDATA PORTVEC);
void KHDLCTIMER(PHDLCDATA PORTVEC);
void KHDLCCLOSE(PHDLCDATA PORTVEC);
BOOL KHDLCTXCHECK();


#else

extern VOID PC120INIT(), DRSIINIT(), TOSHINIT();
extern VOID RLC100INIT(), BAYCOMINIT(), PA0INIT();

extern VOID HDLCTX();
extern VOID HDLCRX();
extern VOID HDLCTIMER();
extern VOID HDLCCLOSE();
extern VOID HDLCTXCHECK();

#endif

extern VOID KISSINIT(), KISSTX(), KISSRX(), KISSTIMER(), KISSCLOSE();


//	VECTORS TO HARDWARE DEPENDENT ROUTINES


VOID * INITCODE[12] = {KISSINIT, 0, 0, 0, 0, 0, 0, LINKINIT, 0, 0, 0, 0};
VOID * TXCODE[12] = {KISSTX, 0, 0, 0, 0, 0, 0, LINKTX, 0, 0, 0, 0};
VOID * RXCODE[12] = {KISSRX, 0, 0, 0, 0, 0, 0, LINKRX, 0, 0, 0, 0};
VOID * TIMERCODE[12] = {KISSTIMER, 0, 0, 0, 0, 0, 0, LINKTIMER, 0, 0, 0, 0};
VOID * CLOSECODE[12] = {KISSCLOSE, 0, 0, 0, 0,0, 0, LINKCLOSE, 0, 0, 0, 0};
VOID * TXCHECKCODE[12] = {KISSTXCHECK, 0, 0, 0, 0, 0, 0, LINKTXCHECK, 0, 0, 0, 0};

extern int BACKGROUND();
extern int L2TimerProc();
extern int L3TimerProc();
extern int L4TimerProc();
extern int L3FastTimer();
extern int StatsTimer();
extern int COMMANDHANDLER();
extern int SDETX();
extern int L4BG();
extern int L3BG();
extern int TNCTimerProc();
extern int PROCESSIFRAME();

int xxxxx = MAXDATA;

BOOL Start()
{
	struct CONFIGTABLE * cfg = xxcfg;
	struct APPLCONFIG * ptr1;
	struct PORTCONTROL * PORT = NULL;
	struct FULLPORTDATA * FULLPORT;		// Including HW Data
	struct _EXTPORTDATA * EXTPORT;
	APPLCALLS * APPL;
	struct ROUTE * ROUTE;
	struct DEST_LIST * DEST;
	struct CMDX * CMD;
	int PortSlot = 1;
	uintptr_t int3;

	unsigned char * ptr2 = 0, * ptr3, * ptr4;
	USHORT * CWPTR;
	int i, n;

	struct ROUTECONFIG * Rcfg;


  int PORTENTRYLEN = sizeof(struct PORTCONTROL);

	// Reinit everything in case of restart

	FREE_Q = 0;
	TRACE_Q = 0;
	IDMSG_Q = 0;
	NUMBEROFPORTS = 0;
	MAXBUFFS = 0;
	QCOUNT = 0;
	NUMBEROFNODES = 0;
	DESTHEADER = 0;
	NODESINPROGRESS = 0;
	CURRENTNODE = 0;
	L3TIMER = 1;						// SEND NODES

	if (cfg->C_NODEALIAS[0] == 0)
		memset(cfg->C_NODEALIAS, ' ', 10);

	TimeLoaded = time(NULL);

	AUTOSAVE = cfg->C_AUTOSAVE;
	
	IPRequired = cfg->C_IP;

	if (cfg->C_MAXHOPS)
		MaxHops = cfg->C_MAXHOPS;

	if (cfg->C_MAXRTT)
		MAXRTT = cfg->C_MAXRTT * 100;

	
	memcpy(MYNODECALL, cfg->C_NODECALL, 10);
	memcpy(MYALIASTEXT, cfg->C_NODEALIAS, 6);
	memcpy(MYALIASLOPPED, cfg->C_NODEALIAS, 10);

	strlop(MYALIASLOPPED, ' ');

	
	//	copy MYCALL to NETROMCALL

	memcpy(MYNETROMCALL, MYNODECALL, 10);
	
	//	if NETROMCALL Defined, use it

	if (cfg->C_NETROMCALL[0] && cfg->C_NETROMCALL[0] != ' ')
		memcpy(MYNETROMCALL, cfg->C_NETROMCALL, 10);

	strlop(MYNETROMCALL, ' ');
	strlop(MYNODECALL, ' ');

	memcpy(NODECALLLOPPED, MYNODECALL, 10);
	strlop(NODECALLLOPPED, ' ');

	APPLCALLTABLE[0].APPLQUAL = BBSQUAL;

	if (cfg->C_WASUNPROTO == 0 && cfg->C_BTEXT)
	{
		char * ptr1 = &cfg->C_BTEXT[0];
		char * ptr2 = BTHDDR.L2DATA;
		int len = 120;

		BTHDDR.LENGTH = 1;			// PID

		while ((*ptr1) && len--)
		{
			*(ptr2++) = *(ptr1++);
			BTHDDR.LENGTH ++;
		}

	}

	OBSINIT = cfg->C_OBSINIT;
	OBSMIN = cfg->C_OBSMIN;
	L3INTERVAL = cfg->C_NODESINTERVAL;
	IDINTERVAL = cfg->C_IDINTERVAL;
	if (IDINTERVAL)
		IDTIMER = 2;

	BTINTERVAL = cfg->C_BTINTERVAL;
	if (BTINTERVAL)
		BTTIMER = 2;


	MINQUAL = cfg->C_MINQUAL;
	FULL_CTEXT = cfg->C_FULLCTEXT;
	L3LIVES = cfg->C_L3TIMETOLIVE;
	L4N2 = cfg->C_L4RETRIES;
	L4DEFAULTWINDOW = cfg->C_L4WINDOW;
	L4T1 = cfg->C_L4TIMEOUT;

  NUMBEROFBUFFERS = cfg->C_BUFFERS;

	PACLEN = cfg->C_PACLEN;
	T3 = cfg->C_T3 * 3;
	L4LIMIT = cfg->C_IDLETIME;
	if (L4LIMIT && L4LIMIT < 120)
		L4LIMIT = 120;					// Don't allow stupidly low
	L2KILLTIME = L4LIMIT * 3;
	L4DELAY = cfg->C_L4DELAY;
	NODE = 1;
	LINKEDFLAG = cfg->C_LINKEDFLAG;
	MAXLINKS = cfg->C_MAXLINKS;
	MAXDESTS = cfg->C_MAXDESTS;
	MAXNEIGHBOURS = cfg->C_MAXNEIGHBOURS;
	MAXCIRCUITS = cfg->C_MAXCIRCUITS;
	HIDENODES = cfg->C_HIDENODES;

	LogL4Connects = cfg->C_LogL4Connects;
	LogAllConnects = cfg->C_LogAllConnects;
	AUTOSAVEMH = cfg->C_SaveMH;
	EventsEnabled = cfg->C_EVENTS;

  // Set up password

  strcpy(PWTEXT, &cfg->C_PASSWORD[1]);
  strlop(PWTEXT, '"');
	PWLen = strlen(PWTEXT);
 
  free(cfg->C_PASSWORD);

	// Get pointers to PASSWORD and APPL1 commands

//	int APPL1 = 0;
//int PASSCMD = 0;

	CMD = &COMMANDS[0];
	n = 0;
	
	for (n = 0; n < NUMBEROFCOMMANDS; n++)
	{
		if (APPL1 == 0 && CMD->String[0] == '*')		// First appl
		{
			APPLS = (char *)CMD;
			APPL1 = n;
		}

		if (PASSCMD == 0 && memcmp(CMD->String, "PASSWORD", 8) == 0)
			PASSCMD = n;
		
		CMD++;
	}

	// Set up Exclude List

	memcpy(ExcludeList, cfg->C_EXCLUDE, 71);


  strlcpy(MQTT_HOST, cfg->C_MQTT_HOST, 80);
  MQTT_PORT = cfg->C_MQTT_PORT;
  strlcpy(MQTT_USER, cfg->C_MQTT_USER, 80);
  strlcpy(MQTT_PASS, cfg->C_MQTT_PASS, 80);

	n = sizeof(struct FULLPORTDATA);

	//	SET UP PORT TABLE

	PortRec = &cfg->C_PORT[0];// (struct PORTCONFIG *)ptr2;

	PORTTABLE = zalloc(n);
	FULLPORT = (struct FULLPORTDATA *)PORTTABLE;

	while (PortRec->PORTNUM)
	{		
		//	SET UP NEXT PORT PTR

		PORT = &FULLPORT->PORTCONTROL;

		PORT->PORTPOINTER = (struct PORTCONTROL *)zalloc(sizeof(struct FULLPORTDATA));

		PORT->PORTNUMBER = (UCHAR)PortRec->PORTNUM;
		PORT->PortSlot = PortSlot++;
		memcpy(PORT->PORTDESCRIPTION, PortRec->ID, 30);

		PORT->PORTTYPE = (char)PortRec->TYPE;

		PORT->PORTINITCODE = INITCODE[PORT->PORTTYPE / 2];	// ADDR OF INIT ROUTINE
		PORT->PORTTXROUTINE = TXCODE[PORT->PORTTYPE / 2];	// ADDR OF INIT ROUTINE
		PORT->PORTRXROUTINE = RXCODE[PORT->PORTTYPE / 2];	// ADDR OF INIT ROUTINE
		PORT->PORTTIMERCODE = TIMERCODE[PORT->PORTTYPE / 2];	// ADDR OF INIT ROUTINE
		PORT->PORTCLOSECODE = CLOSECODE[PORT->PORTTYPE / 2];	// ADDR OF INIT ROUTINE
		PORT->PORTTXCHECKCODE = TXCHECKCODE[PORT->PORTTYPE / 2];	// ADDR OF INIT ROUTINE
	

		PORT->PROTOCOL = (char)PortRec->PROTOCOL;
		PORT->IOBASE = PortRec->IOADDR;

		if (PortRec->SerialPortName[0])
			PORT->SerialPortName = _strdup(PortRec->SerialPortName);
		else
		{
			if (PORT->IOBASE > 0 && PORT->IOBASE < 256)
			{
				char Name[80];
#ifndef WIN32
				sprintf(Name, "com%d", PORT->IOBASE);
#else
				sprintf(Name, "COM%d", PORT->IOBASE);
#endif	
				PORT->SerialPortName = _strdup(Name);
			}
			else
				PORT->SerialPortName = _strdup("NOPORT");

		}
		PORT->INTLEVEL = (char)PortRec->INTLEVEL;
		PORT->BAUDRATE = PortRec->SPEED;
	
		if (PORT->BAUDRATE == 49664)
			PORT->BAUDRATE = (int)115200;

		PORT->CHANNELNUM = (char)PortRec->CHANNEL;
		PORT->PORTQUALITY = (UCHAR)PortRec->QUALITY;
		PORT->NormalizeQuality = !PortRec->NoNormalize;
		PORT->IgnoreUnlocked = PortRec->IGNOREUNLOCKED;
		PORT->INP3ONLY = PortRec->INP3ONLY;

		PORT->PORTWINDOW = (UCHAR)PortRec->MAXFRAME;

		if (PortRec->PROTOCOL == 0 || PORT->PORTTYPE == 22)		// KISS or I2C
			PORT->PORTTXDELAY = PortRec->TXDELAY /10;
		else
			PORT->PORTTXDELAY = PortRec->TXDELAY;

		if (PortRec->PROTOCOL == 0 || PORT->PORTTYPE == 22)		// KISS or I2C
			PORT->PORTSLOTTIME = (UCHAR)PortRec->SLOTTIME / 10;
		else
			PORT->PORTSLOTTIME = (UCHAR)PortRec->SLOTTIME;
		
		PORT->PORTPERSISTANCE = (UCHAR)PortRec->PERSIST;
		PORT->FULLDUPLEX = (UCHAR)PortRec->FULLDUP;

		PORT->SOFTDCDFLAG = (UCHAR)PortRec->SOFTDCD;
		PORT->PORTT1 = PortRec->FRACK / 333;
		PORT->PORTT2 = PortRec->RESPTIME /333;
		PORT->PORTN2 = (UCHAR)PortRec->RETRIES;

		PORT->PORTPACLEN = (UCHAR)PortRec->PACLEN;
		PORT->QUAL_ADJUST = (UCHAR)PortRec->QUALADJUST;
	
		PORT->DIGIFLAG = PortRec->DIGIFLAG;
		if (PortRec->DIGIPORT && CanPortDigi(PortRec->DIGIPORT))
			PORT->DIGIPORT = PortRec->DIGIPORT;
		PORT->DIGIMASK = PortRec->DIGIMASK;
		PORT->USERS = (UCHAR)PortRec->USERS;

		// PORTTAILTIME - if KISS, set a default, and cnvert to ticks

		if (PORT->PORTTYPE == 0)
		{
			if (PortRec->TXTAIL)
				PORT->PORTTAILTIME = PortRec->TXTAIL / 10;
			else
				PORT->PORTTAILTIME = 3;		// 10ths
		}
		else

			//;	ON HDLC, TAIL TIMER IS USED TO HOLD RTS FOR 'CONTROLLED FULL DUP' - Val in seconds

			PORT->PORTTAILTIME = (UCHAR)PortRec->TXTAIL;

		PORT->PORTBBSFLAG = (char)PortRec->ALIAS_IS_BBS;
		PORT->PORTL3FLAG = (char)PortRec->L3ONLY;
		PORT->KISSFLAGS = PortRec->KISSOPTIONS;
		PORT->PORTINTERLOCK = (UCHAR)PortRec->INTERLOCK;
		PORT->NODESPACLEN = (UCHAR)PortRec->NODESPACLEN;
		PORT->TXPORT = (UCHAR)PortRec->TXPORT;

		PORT->PORTMINQUAL = PortRec->MINQUAL;

		if (PortRec->MAXDIGIS)
			PORT->PORTMAXDIGIS = PortRec->MAXDIGIS;
		else
			PORT->PORTMAXDIGIS = 8;

		PORT->PortNoKeepAlive = PortRec->DefaultNoKeepAlives;
		PORT->PortUIONLY = PortRec->UIONLY;
		PORT->TXPORT = (UCHAR)PortRec->TXPORT;

		//	SET UP CWID

		if (PortRec->CWIDTYPE == 'o')
			PORT->CWTYPE = 'O';
		else
			PORT->CWTYPE = PortRec->CWIDTYPE;

		ptr2 = &PortRec->CWID[0];
		CWPTR = &PORT->CWID[0];
		

		//	SEE IF LINK CALLSIGN/ALIAS SPECIFIED

		if (PortRec->PORTCALL[0])
			ConvToAX25(PortRec->PORTCALL, PORT->PORTCALL);

		if (PortRec->PORTALIAS[0])
			ConvToAX25(PortRec->PORTALIAS, PORT->PORTALIAS);

		if (PortRec->PORTALIAS2[0])
			ConvToAX25(PortRec->PORTALIAS2, PORT->PORTALIAS2);

		if (PortRec->DLLNAME[0])
		{
			EXTPORT = (struct _EXTPORTDATA *)PORT;
			memcpy(EXTPORT->PORT_DLL_NAME, PortRec->DLLNAME, 16);
		}
		if (PortRec->BCALL[0])
			ConvToAX25(PortRec->BCALL, PORT->PORTBCALL);

		PORT->XDIGIS = PortRec->XDIGIS;		// Crossband digi aliases

		PORT->ListenPort = PortRec->ListenPort;

		if (PortRec->TCPPORT)
		{
			PORT->KISSTCP = TRUE;
			PORT->IOBASE = PortRec->TCPPORT;
			if (PortRec->IPADDR == 0)
				PORT->KISSSLAVE = TRUE;
		}

		if (PortRec->WL2K)
			memcpy(&PORT->WL2KInfo, PortRec->WL2K, sizeof(struct WL2KInfo));

		PORT->RIGPort = PortRec->RIGPORT;

		if (PortRec->HavePermittedAppls)
			PORT->PERMITTEDAPPLS = PortRec->PERMITTEDAPPLS;
		else
			PORT->PERMITTEDAPPLS = 0xffffffff;		// Default to all

		PORT->Hide = PortRec->Hide;

		PORT->SmartIDInterval = PortRec->SmartID;

		if (PortRec->KissParams && (PORT->PORTTYPE == 0 || PORT->PORTTYPE == 22))
		{
			struct KISSINFO * KISS = (struct KISSINFO *)PORT;
			UCHAR KissString[128];
			int KissLen = 0;
			unsigned char * Kissptr = KissString;
			char * ptr;
			char * Context;

			ptr = strtok_s(PortRec->KissParams, " ", &Context);

			while (ptr && ptr[0] && KissLen < 120)
			{
				*(Kissptr++) = atoi (ptr);
				KissLen++;
				ptr = strtok_s(NULL, " ", &Context);
			}

			KISS->KISSCMD = malloc(256);

			KISS->KISSCMDLEN = KissEncode(KissString, KISS->KISSCMD, KissLen);
			KISS->KISSCMD = realloc(KISS->KISSCMD, KISS->KISSCMDLEN);
		}

		PORT->SendtoM0LTEMap = PortRec->SendtoM0LTEMap;
		PORT->PortFreq = PortRec->PortFreq;

		PORT->M0LTEMapInfo = PortRec->M0LTEMapInfo;

		PORT->QtSMPort = PortRec->QtSMPort;

		if (PortRec->BBSFLAG)						// Appl 1 not permitted - BBSFLAG=NOBBS
			PORT->PERMITTEDAPPLS &= 0xfffffffe;		// Clear bottom bit

			
		//	SEE IF PERMITTED LINK CALLSIGNS SPECIFIED

		ptr2 = &PortRec->VALIDCALLS[0];

		if (*(ptr2))
		{
			PORT->PERMITTEDCALLS = zalloc(256);
			ptr3 = PORT->PERMITTEDCALLS;	

			while (*(ptr2) > 32)
			{
				ConvToAX25(ptr2, ptr3);
				ptr3 += 7;
				PORT->PORTPOINTER = (struct PORTCONTROL *)ptr3;
				if (strchr(ptr2, ','))
				{
					ptr2 = strchr(ptr2, ',');
					ptr2++;
				}
				else
					break;
			}
		}

		//	SEE IF PORT UNPROTO ADDR SPECIFIED

		ptr2 = &PortRec->UNPROTO[0];

		if (*(ptr2))
		{
			PORT->PORTUNPROTO = zalloc(80);
			ptr3 = PORT->PORTUNPROTO;

			while (*(ptr2) > 32)
			{
				ConvToAX25(ptr2, ptr3);
				ptr3 += 7;
				PORT->PORTPOINTER = (struct PORTCONTROL *)ptr3;
				if (strchr(ptr2, ','))
				{
					ptr2 = strchr(ptr2, ',');
					ptr2++;
				}
				else
					break;
			}
		}

		//	ADD MH AREA IF NEEDED

		if (PortRec->MHEARD != 'N')
		{
			NEEDMH = 1;								// Include MH in Command List
			PORT->PORTMHEARD = zalloc((MHENTRIES + 1) * sizeof(MHSTRUC));
		}

		PortRec++;
		NUMBEROFPORTS ++;
		FULLPORT = (struct FULLPORTDATA *)PORT->PORTPOINTER;
	}

	free(PORT->PORTPOINTER);
	PORT->PORTPOINTER = NULL;		// End of list


	//	SET UP VARIOUS CONTROL TABLES

	LINKS = (struct _LINKTABLE *)zalloc(MAXLINKS * sizeof(struct _LINKTABLE));
	DESTS = (struct DEST_LIST *)zalloc(MAXDESTS * sizeof(struct DEST_LIST));
	NEIGHBOURS = (struct ROUTE *)zalloc(MAXNEIGHBOURS * sizeof(struct ROUTE));
	L4TABLE = (TRANSPORTENTRY *)zalloc(MAXCIRCUITS * sizeof(TRANSPORTENTRY));

	//	SET UP DEFAULT ROUTES LIST

	Rcfg = &cfg->C_ROUTE[0];

	ROUTE = NEIGHBOURS;

	while (Rcfg->call[0])
	{
		int FRACK;
		char * VIA;
		char axcall[8];
		
		ConvToAX25(Rcfg->call, ROUTE->NEIGHBOUR_CALL);

		// if VIA convert digis

		VIA = strstr(Rcfg->call, "VIA");

		if (VIA)
		{
			VIA += 4;

			if (ConvToAX25(VIA, axcall))
			{
				memcpy(ROUTE->NEIGHBOUR_DIGI1, axcall, 7);

				VIA = strchr(VIA, ' ');
	
				if (VIA)
				{
					VIA++;

					if (ConvToAX25(VIA, axcall))
						memcpy(ROUTE->NEIGHBOUR_DIGI2, axcall, 7);

				}
			}
		}
		
		ROUTE->NEIGHBOUR_QUAL = Rcfg->quality;
		ROUTE->NEIGHBOUR_PORT = Rcfg->port;
		
		PORT = GetPortTableEntryFromPortNum(ROUTE->NEIGHBOUR_PORT);

		if (Rcfg->pwind & 0x40)
			ROUTE->NoKeepAlive = 1;
		else
			if (PORT != NULL)
				ROUTE->NoKeepAlive = PORT->PortNoKeepAlive;

		if (Rcfg->pwind & 0x80 || (PORT && PORT->INP3ONLY))
		{
			ROUTE->INP3Node = 1;
			ROUTE->NoKeepAlive = 0;			// Cant have INP3 and NOKEEPALIVES
		}

		ROUTE->NBOUR_MAXFRAME = Rcfg->pwind & 0x3f;

		FRACK = Rcfg->pfrack;
		ROUTE->NBOUR_FRACK = FRACK / 333; 
		ROUTE->NBOUR_PACLEN = Rcfg->ppacl;
		ROUTE->OtherendsRouteQual = ROUTE->OtherendLocked = Rcfg->farQual;

		ROUTE->NEIGHBOUR_FLAG = 1;			// Locked
		
		Rcfg++;
		ROUTE++;
	}
	//	SET UP INFO MESSAGE

	if (cfg->C_INFOMSG)
		INFOMSG = cfg->C_INFOMSG;
	else
		INFOMSG = zalloc(2);


	//	SET UP CTEXT MESSAGE

	if (cfg->C_CTEXT)
		CTEXTMSG = cfg->C_CTEXT;
	else
		CTEXTMSG = zalloc(2);

	CTEXTLEN = strlen(CTEXTMSG);

	//	SET UP ID MESSAGE

	IDHDDR.DEST[0] = 'I'+'I';
	IDHDDR.DEST[1] = 'D'+'D';
	IDHDDR.DEST[2] = 0x40;
	IDHDDR.DEST[3] = 0x40;
	IDHDDR.DEST[4] = 0x40;
	IDHDDR.DEST[5] = 0x40;
	IDHDDR.DEST[6] = 0xe0;		//	; ID IN AX25 FORM

	IDHDDR.CTL = 3;
	IDHDDR.PID = 0xf0;

	ptr2 = cfg->C_IDMSG;
	ptr3 = &IDHDDR.L2DATA[0];

	while (ptr2 && (*ptr2))
	{
		*(ptr3++) = *(ptr2++);
	}

	IDHDDR.LENGTH = (int)(ptr3 - (unsigned char *)&IDHDDR);

	//Consoleprintf("PORTS %p LINKS %p DESTS %p ROUTES %p L4 %p BUFFERS %p\n",
	//	PORTTABLE, LINKS, DESTS, NEIGHBOURS, L4TABLE, BUFFERPOOL);

	Debugprintf("PORTS %p LINKS %p DESTS %p ROUTES %p L4 %p Free Heap %d",
		PORTTABLE, LINKS, DESTS, NEIGHBOURS, L4TABLE, getFreeHeap());

  if (NUMBEROFBUFFERS > 250)
    NUMBEROFBUFFERS = 250;

	i = NUMBEROFBUFFERS;

	NUMBEROFBUFFERS = 0;

	while (i--)
	{
		WorkBuffer = zalloc(BUFFALLOC);
		Bufferlist[NUMBEROFBUFFERS] = (void **)WorkBuffer;

		ReleaseBuffer((UINT *)WorkBuffer);
		WorkBuffer += BUFFALLOC;		// was BUFFLEN

		NUMBEROFBUFFERS++;
		MAXBUFFS++;

    if (getFreeHeap() < 50000)
      break;
	}

	//	Copy Bridge Map

	memcpy(BridgeMap, &cfg->CfgBridgeMap, sizeof(BridgeMap));

//	MOV	EAX,_NEXTFREEDATA
//	CALL	HEXOUT

	//	SET UP OUR CALLIGN(S)

	ConvToAX25(MYNETROMCALL, NETROMCALL);

	ConvToAX25(MYNODECALL, MYCALL);
	memcpy(&IDHDDR.ORIGIN[0], MYCALL, 7);
	IDHDDR.ORIGIN[6] |= 0x61;			// SET CMD END AND RESERVED BITS

	ConvToAX25(MYALIASTEXT, MYALIAS);

	//	SET UP INITIAL DEST ENTRY FOR APPLICATIONS (IF BOTH NODE AND BBS NEEDED)
	// Actually wrong - we need to set up application node entries even if node = 0
	DEST = DESTS;

	//	If NODECALL isn't same as NETROMCALL, Add Dest Entry for NODECALL

	if (memcmp(NETROMCALL, MYCALL, 7) != 0)
	{
		memcpy(DEST->DEST_CALL, MYCALL, 7);
		memcpy(DEST->DEST_ALIAS, MYALIASTEXT, 6);

		DEST->DEST_STATE = 0x80;	// SPECIAL ENTRY
		DEST->NRROUTE[0].ROUT_QUALITY = 255;
		DEST->NRROUTE[0].ROUT_OBSCOUNT = 255;
		DEST++;
		NUMBEROFNODES++;
	}

	if (BBS)
	{
		// Add Application Entries

		APPL = &APPLCALLTABLE[0]; 
		i = NumberofAppls;

		while (i--)
		{
			if (APPL->APPLQUAL)
			{
				memcpy(DEST->DEST_CALL, APPL->APPLCALL, 13);
				DEST->DEST_STATE = 0x80;	// SPECIAL ENTRY
				DEST->NRROUTE[0].ROUT_QUALITY = (UCHAR)APPL->APPLQUAL;
				DEST->NRROUTE[0].ROUT_OBSCOUNT = 255;
				APPL->NODEPOINTER = DEST;

				DEST++;

				NUMBEROFNODES++;
			}
			APPL++;
		}
	}

	// Read Node and MH Recovery Files

	ReadNodes();

	if (AUTOSAVEMH)
		ReadMH();			// Only if AutoSave configured

	//	set up stream number in BPQHOSTVECTOR

	for (i = 0; i < 64; i++)
	{
		BPQHOSTVECTOR[i].HOSTSTREAM = i + 1;
	}

	memcpy(MYCALLWITHALIAS, MYCALL, 7);
	memcpy(&MYCALLWITHALIAS[7], MYALIASTEXT, 6);

	// Set random start value for NETROM Session ID

	NEXTID = (rand() % 254) + 1;

	GetPortCTEXT(0, 0, 0, 0);


	last15Mins = CurrentSecs = lastSlowSecs = time(NULL);

  free(xxcfg);
  
	return 0;
}

BOOL CompareCalls(UCHAR * c1, UCHAR * c2)
{
	//	COMPARE AX25 CALLSIGNS IGNORING EXTRA BITS IN SSID

	if (memcmp(c1, c2, 6))
		return FALSE;			// No Match

	if ((c1[6] & 0x1e) == (c2[6] & 0x1e))
		return TRUE;

	return FALSE;
}
BOOL CompareAliases(UCHAR * c1, UCHAR * c2)
{
	//	COMPARE first 6 chars of AX25 CALLSIGNS

	if (memcmp(c1, c2, 6))
		return FALSE;			// No Match

	return TRUE;
}
BOOL FindNeighbour(UCHAR * Call, int Port, struct ROUTE ** REQROUTE)
{
	struct ROUTE * ROUTE = NEIGHBOURS;
	struct ROUTE * FIRSTSPARE = NULL;
	int n = MAXNEIGHBOURS;

	while (n--)
	{
		if (ROUTE->NEIGHBOUR_CALL[0] == 0)		// Spare
			if (FIRSTSPARE == NULL)
				FIRSTSPARE = ROUTE;

		if (ROUTE->NEIGHBOUR_PORT != Port)
		{
			ROUTE++;
			continue;
		}
		if (CompareCalls(ROUTE->NEIGHBOUR_CALL, Call))
		{
			*REQROUTE = ROUTE;
			return TRUE;
		}
		ROUTE++;
	}

	//	ENTRY NOT FOUND - FIRSTSPARE HAS FIRST FREE ENTRY, OR ZERO IF TABLE FULL

	*REQROUTE = FIRSTSPARE;
	return FALSE;
}

BOOL FindDestination(UCHAR * Call, struct DEST_LIST ** REQDEST)
{
	struct DEST_LIST * DEST = DESTS;
	struct DEST_LIST * FIRSTSPARE = NULL;
	int n = MAXDESTS;

	while (n--)
	{
		if (DEST->DEST_CALL[0] == 0)		// Spare
		{
			if (FIRSTSPARE == NULL)
				FIRSTSPARE = DEST;

			DEST++;
			continue;
		}
		if (CompareCalls(DEST->DEST_CALL, Call))
		{
			*REQDEST = DEST;
			return TRUE;
		}
		DEST++;
	}

	//	ENTRY NOT FOUND - FIRSTSPARE HAS FIRST FREE ENTRY, OR ZERO IF TABLE FULL

	*REQDEST = FIRSTSPARE;
	return FALSE;
}

extern UCHAR BPQDirectory[];

#define LINE_MAX 256


// Reload saved MH file

VOID ReadMH()
{
	char line[LINE_MAX];
	UCHAR axcall[7];
	char * ptr, *digi, *locptr, *locend;
	char * Context, * Context2;
	char seps[] = " \n";
	int Port;
	struct PORTCONTROL * PORT = NULL;
	MHSTRUC * MH = NULL;
	int count = MHENTRIES;
	char * Digiptr;
	BOOL Digiused;
	char * HasStar;
  void * file;
	// Open MH File

  file = picoOpenFile("MHSave.txt");
  
  if (file == 0)
  {
		WritetoConsoleLocal("MH file recovery file MHSave.txt not found - Continuing without it\n");
		return;
	}

	// Read the saved MH file

	while (picoGetLine(file, line, LINE_MAX))
	{
		if (memcmp(line, "Port:", 5) == 0)
		{
			Port = atoi(&line[5]);
			PORT = GetPortTableEntryFromPortNum(Port);
			if (PORT)				
				MH = PORT->PORTMHEARD;
			else
				MH = NULL;
			
			continue;
		}

		// 1548777630 N9LYA-8    Jan 29 16:00:30 	
		
		if (MH)
		{
			ptr = strtok_s(line, seps, &Context);

			if (ptr == NULL)
				continue;
			
			MH->MHTIME = atoi(ptr);
			
			ptr = strtok_s(NULL, seps, &Context);

			if (ptr == NULL)
				continue;
			
			MH->MHCOUNT = atoi(ptr);

			// Get LOC and Freq First before strtok messes with line

			locptr = strchr(Context, '|'); 

			if (locptr == NULL)
				continue;

			locend = strchr(++locptr, '|');

			if (locend == NULL)
				continue;

			if ((locend - locptr) == 6)
					
				memcpy(MH->MHLocator, locptr, 6);

			locend++;

			strlop(locend, '\n');

			if (strlen(locend) < 12)
					
				strcpy(MH->MHFreq, locend);

			ptr = strtok_s(NULL, "|\n", &Context);

			if (ptr == NULL)
				continue;
		
			if (ConvToAX25(ptr, axcall) == FALSE)
				continue;				// Duff

			memcpy(MH->MHCALL, axcall, 7);	

			if (ptr[10] != ' ')
				MH->MHDIGI = ptr[10];
			
			//	SEE IF ANY DIGIS

			digi = strstr(ptr, " via ");
			
			if (digi)
			{
				digi = digi + 5;
				Digiptr = &MH->MHDIGIS[0][0];

				if (strchr(ptr, '*'))
					Digiused = 1;		// At least one digi used
				else
					Digiused = 0;
				
				digi = strtok_s(digi, ",\n", &Context2);
				
				while(digi)
				{
					HasStar = strlop(digi, '*');
					
					if (ConvToAX25(digi, axcall) == FALSE)
						break;

					memcpy(Digiptr, axcall, 7);

					if (Digiused)
						Digiptr[6] |= 0x80;		// Set used
					
					if (HasStar)
						Digiused = 0;			// Only last used has *

					Digiptr += 7;

					digi = strtok_s(NULL, ",/n", &Context2);
				}

				*(--Digiptr) |= 1;		// Set end of address on last

			}
			else
			{
				// No Digis
			
				MH->MHCALL[6] |= 1;		// Set end of address
			}

			MH++;
		}
	}

	picoCloseFile(file);
	return;
}

VOID ReadNodes()
{
	char FN[260] = "BPQNODES.dat";
	FILE *fp;
	char line[LINE_MAX];
	UCHAR axcall[7];
	char * ptr;
	char * Context;
	char seps[] = " \r";
	int Port, Qual;
	struct PORTCONTROL * PORT;
  void * file;

	// Set up pointer to BPQNODES file

	file = picoOpenFile(FN);
  
  if (picoOpenFile(FN) == 0)
	{
		WritetoConsoleLocal("Route/Node recovery file BPQNODES.dat not found - Continuing without it\n");
		return;
	}

	// Read the saved ROUTES/NODES file

	while (picoGetLine(file, line, LINE_MAX))
	{
 		if (memcmp(line, "ROUTE ADD", 9) == 0)
		{
			struct ROUTE * ROUTE = NULL;

			//	FORMAT IS ROUTE ADD CALLSIGN  PORT QUAL (VIA .... 

			ptr = strtok_s(&line[10], seps, &Context);

			if (ConvToAX25(ptr, axcall) == FALSE)
				continue;				// DUff

			ptr = strtok_s(NULL, seps, &Context);
			if (ptr == NULL) continue;
			Port = atoi(ptr);

			PORT = GetPortTableEntryFromPortNum(Port);

			if (PORT == NULL)
				continue;				// Port has gone

			if (FindNeighbour(axcall, Port, &ROUTE))
				continue;				// Already added from ROUTES:

			if (ROUTE == NULL)
				continue;				// Tsble Full

			memcpy(ROUTE->NEIGHBOUR_CALL, axcall, 7);
			ROUTE->NEIGHBOUR_PORT = Port;

			ptr = strtok_s(NULL, seps, &Context);
			if (ptr == NULL) continue;
			Qual = atoi(ptr);

			ROUTE->NEIGHBOUR_QUAL = Qual;

			ptr = strtok_s(NULL, seps, &Context);	// MAXFRAME
			if (ptr == NULL) continue;

			// I don't thinlk we should load locked flag from save file - only from config

			// But need to parse it until I stop saving it

			if (ptr[0] == '!')
			{
//				ROUTE->NEIGHBOUR_FLAG = 1;			// LOCKED ROUTE
				ptr = strtok_s(NULL, seps, &Context);
				if (ptr == NULL) continue;
			}

			//	SEE IF ANY DIGIS

			if (ptr[0] == 'V')
			{
				ptr = strtok_s(NULL, seps, &Context);
				if (ptr == NULL) continue;

				if (ConvToAX25(ptr, axcall) == FALSE)
					continue;				// DUff

				memcpy(ROUTE->NEIGHBOUR_DIGI1, axcall, 7);

				ROUTE->NEIGHBOUR_FLAG = 1;			// LOCKED ROUTE - Digi'ed routes must be locked

				ptr = strtok_s(NULL, seps, &Context);
				if (ptr == NULL) continue;

				// See if another digi or MAXFRAME

				if (strlen(ptr) > 2)
				{
					if (ConvToAX25(ptr, axcall) == FALSE)
						continue;				// DUff

					memcpy(ROUTE->NEIGHBOUR_DIGI2, axcall, 7);

					ptr = strtok_s(NULL, seps, &Context);
					if (ptr == NULL) continue;
				}
			}

			Qual = atoi(ptr);
			ROUTE->NBOUR_MAXFRAME = Qual;

			ptr = strtok_s(NULL, seps, &Context);	// FRACK
			if (ptr == NULL) continue;
			Qual = atoi(ptr);
			ROUTE->NBOUR_FRACK = Qual;

			ptr = strtok_s(NULL, seps, &Context);	// PACLEN
			if (ptr == NULL) continue;
			Qual = atoi(ptr);
			ROUTE->NBOUR_PACLEN = Qual;
	
			ptr = strtok_s(NULL, seps, &Context);	// INP3
			if (ptr == NULL) continue;
			Qual = atoi(ptr);

			//	We now take Nokeepalives from the PORT, unless specifically
			//	Requested

			ROUTE->NoKeepAlive = PORT->PortNoKeepAlive;

			if (Qual & 4)
				ROUTE->NoKeepAlive = TRUE;

			if ((Qual & 1) || PORT->INP3ONLY)
			{
				ROUTE->NoKeepAlive = FALSE;
				ROUTE->INP3Node = TRUE;
			}

			ptr = strtok_s(NULL, seps, &Context);	// INP3
			if (ptr == NULL) continue;

			if (ROUTE->NEIGHBOUR_FLAG == 0 || ROUTE->OtherendLocked == 0);		// Not LOCKED ROUTE
				ROUTE->OtherendsRouteQual = atoi(ptr);

			continue;
		}

		if (memcmp(line, "NODE ADD", 8) == 0)
		{
			//	FORMAT IS NODE ADD ALIAS:CALL ROUTE QUAL

			dest_list * DEST = NULL;
			struct ROUTE * ROUTE = NULL;
			char * ALIAS;
			char FULLALIAS[6] = "      ";
			int SavedOBSINIT = OBSINIT;

			if (line[9] == ':')
			{
				// No alias

				Context = &line[10];
			}
			else
			{
				ALIAS = strtok_s(&line[9], ":", &Context);

				if (ALIAS == NULL)
					continue;

				memcpy(FULLALIAS, ALIAS, (int)strlen(ALIAS));
			}

			ptr = strtok_s(NULL, seps, &Context);
			if (ptr == NULL)
				continue;

			if (ConvToAX25(ptr, axcall) == FALSE)
				continue;				// Duff

			if (CompareCalls(axcall, MYCALL))
				continue;				// Shoiuldn't happen, but to be safe!

			if (FindDestination(axcall, &DEST))
				continue;

			if (DEST == NULL)
				continue;				// Tsble Full

			memcpy(DEST->DEST_CALL, axcall, 7);
			memcpy(DEST->DEST_ALIAS, FULLALIAS, 6);

			NUMBEROFNODES++;
RouteLoop:
			//	GET NEIGHBOURS FOR THIS DESTINATION - CALL PORT QUAL

			ptr = strtok_s(NULL, seps, &Context);
			if (ptr == NULL) continue;

			if (ConvToAX25(ptr, axcall) == FALSE)
				continue;				// DUff

			ptr = strtok_s(NULL, seps, &Context);
			if (ptr == NULL) continue;
			Port = atoi(ptr);

			ptr = strtok_s(NULL, seps, &Context);
			if (ptr == NULL) continue;
			Qual = atoi(ptr);

			if (Context[0] == '!')
			{
				OBSINIT = 255;			//; SPECIAL FOR LOCKED
			}
		
			if (FindNeighbour(axcall, Port, &ROUTE))
			{
				PROCROUTES(DEST, ROUTE, Qual);
			}

			OBSINIT = SavedOBSINIT;

			goto RouteLoop;
		}
	}

	picoCloseFile(file);
	return;
}

int DelayBuffers = 0;


VOID TIMERINTERRUPT()
{
	// Main Processing loop - CALLED EVERY 100 MS

	int i;
	struct PORTCONTROL * PORT = PORTTABLE;
	PMESSAGE Buffer;
	struct _LINKTABLE * LINK;
	struct _MESSAGE * Message;
	int toPort;

	CurrentSecs =  time(NULL);

	BGTIMER++;
	REALTIMETICKS++;
	L2TIMERFLAG++;		// INCREMENT FLAG FOR BG
	L3TIMERFLAG++;		// INCREMENT FLAG FOR BG
	L4TIMERFLAG++;		// INCREMENT FLAG FOR BG

//	CALL PORT TIMER ROUTINES

	for (i = 0; i < NUMBEROFPORTS; i++)
	{	
		PORT->PORTTIMERCODE(PORT);

		// Check Smart ID timer

		if (PORT->SmartIDNeeded && PORT->SmartIDNeeded < time(NULL))
			SendSmartID(PORT);

		PORT = PORT->PORTPOINTER;
	}

	//	CHECK FOR TIMER ACTIVITY

	if (L2TIMERFLAG >= 3)
	{
		L2TIMERFLAG -= 3;
		L2TimerProc();					// 300 mS
	}

	if (CurrentSecs - lastSlowSecs >= 60)		// 1 PER  MIN
	{
		lastSlowSecs = CurrentSecs;

		L3TimerProc();
		StatsTimer();

		// Call Mode Map support routine

/*
		if (QCOUNT < 200)
		{
			if (DelayBuffers == 0)
			{
				FindLostBuffers();
				DelayBuffers = 10;
			}
			else
			{
				DelayBuffers--;
			}
		}
*/
	}

  if (CurrentSecs - last15Mins >= 900)		// 1 PER  15 MIN
	{
		last15Mins = CurrentSecs;
		Debugprintf("BPQ32 Heartbeat: Buffers %d Free Heap %d", QCOUNT, getFreeHeap());
  }

	if (L4TIMERFLAG >= 10)				// 1 PER SEC
	{
		L4TIMERFLAG -= 10;

		L3FastTimer();
		L4TimerProc();
	}

	// SEE IF ANY FRAMES TO TRACE

	Buffer = Q_REM(&TRACE_Q);

	while (Buffer)
	{
		//	IF BUFFER HAS A LINK TABLE ENTRY ON END, RESET TIMEOUT

		LINK = Buffer->Linkptr;

		if (LINK)
		{
			if (LINK->L2TIMER)
				LINK->L2TIMER = LINK->L2TIME;

			Buffer->Linkptr = 0;	// CLEAR FLAG FROM BUFFER
		}

		Message = (struct _MESSAGE *)Buffer;
		Message->PORT |= 0x80;			// Set TX Bit
	
		BPQTRACE(Message, FALSE);		// Dont send TX'ed frames to APRS
		ReleaseBuffer(Buffer);

		Buffer = Q_REM(&TRACE_Q);
	}

	//	CHECK FOR MESSAGES RECEIVED FROM COMMS LINKS

	PORT = PORTTABLE;

	for (i = 0; i < NUMBEROFPORTS; i++)
	{	
		int Sent = 0;

		CURRENTPORT = PORT->PORTNUMBER;		 // PORT NUMBER
		CURRENTPORTPTR = PORT;

		Buffer = (PMESSAGE)Q_REM((void *)&PORT->PORTRX_Q);

		while (Buffer)
		{
			Message = (struct _MESSAGE *) Buffer;

			//	TIME STAMP IT
	
			time(&Message->Timestamp);

			Message->PORT = CURRENTPORT;
			
			// Bridge if requested

			for (toPort = 1; toPort <= MaxBPQPortNo; toPort++)
			{
				if (BridgeMap[CURRENTPORT][toPort])
				{
					MESSAGE * BBuffer = GetBuff();
					struct PORTCONTROL * BPORT;

					if (BBuffer)
					{
						memcpy(BBuffer, Message, Message->LENGTH);
						BBuffer->PORT = toPort;
						BPORT = GetPortTableEntryFromPortNum(toPort);
					
						if (BPORT)
						{
							if (BPORT->SmartIDInterval && BPORT->SmartIDNeeded == 0)
							{
								// Using Smart ID, but none scheduled

								BPORT->SmartIDNeeded = time(NULL) + BPORT->SmartIDInterval;
							}
							PUT_ON_PORT_Q(BPORT, BBuffer);
						}
						else
							ReleaseBuffer(BBuffer);
					}	
				}
			}

			L2Routine(PORT, Buffer);

			Buffer = (PMESSAGE)Q_REM((void *)&PORT->PORTRX_Q);
			continue;
		}

		// End of RX_Q

		Sent = 0;

		while (PORT->PORTTX_Q && Sent < 5)
		{
			int ret;
			void * PACTORSAVEQ;

			Buffer = PORT->PORTTX_Q;
			Message = (struct _MESSAGE *) Buffer;
			
			ret = PORT->PORTTXCHECKCODE(PORT, Message->PORT);

			// Busy but not connected means TNC has gone - clear queue

			if (ret == 1)
			{
				MESSAGE * Buffer = (PMESSAGE)Q_REM((void *)&PORT->PORTTX_Q);

				if (Buffer == 0)
					break;						// WOT!!

//				Debugprintf("Busy but not connected - discard message %s", Buffer->L2DATA);

				ReleaseBuffer(Buffer);
				break;
			}
	
			ret = ret & 0xff;			// Only check bottom byte

			if (ret == 0)		// Not busy
			{
				MESSAGE * Buffer = (PMESSAGE)Q_REM((void *)&PORT->PORTTX_Q);

				if (Buffer == 0)
					break;						// WOT!!

				if (PORT->PORTDISABLED)
				{
					ReleaseBuffer(Buffer);
					break;
				}

				PORT->L2FRAMESSENT++;

				PORT->PORTTXROUTINE(PORT, Buffer);
				Sent++;

				continue;
			}

			// If a Pactor Port, some channels could be busy whilst others are not.

			if (PORT->PROTOCOL != 10)
				break;					// BUSY
		
			//	Try passing any other messages on the queue to the node.

			PACTORSAVEQ = 0;	

PACTORLOOP:

			Buffer = PORT->PORTTX_Q;

			if (Buffer == NULL)
				goto ENDOFLIST;
	
			Message = (struct _MESSAGE *) Buffer;
			ret = PORT->PORTTXCHECKCODE(PORT, Message->PORT);
			ret = ret & 0xff;			// Only check bottom byte
		
			if (ret)		// Busy
			{
				//	Save it
				
				Buffer = (PMESSAGE)Q_REM((void *)&PORT->PORTTX_Q);
				C_Q_ADD(&PACTORSAVEQ, Buffer);
				goto PACTORLOOP;
			}

			Buffer = (PMESSAGE)Q_REM((void *)&PORT->PORTTX_Q);

			if (PORT->PORTDISABLED)
			{
				ReleaseBuffer(Buffer);
				goto PACTORLOOP;
			}

			PORT->L2FRAMESSENT++;

			PORT->PORTTXROUTINE(PORT, Buffer);
			Sent++;

			if (Sent < 5)
				goto PACTORLOOP;			// SEE IF MORE

ENDOFLIST:
			//	Move the saved frames back onto Port Q

			PORT->PORTTX_Q = PACTORSAVEQ;
			break;
		}

		PORT->PORTRXROUTINE(PORT);			// SEE IF MESSAGE RECEIVED
		PORT = PORT->PORTPOINTER;
	}

/*
;
;	CHECK FOR INCOMING MESSAGES ON LINK CONTROL TABLE -
;	   BY NOW ONLY 'I' FRAMES WILL BE PRESENT -
;	   LEVEL 2 PROTOCOL HANDLING IS DONE IN MESSAGE RECEIVE CODE
;	   AND LINK HANDLING INTERRUPT ROUTINES
;
*/

	LINK = LINKS;
	i = MAXLINKS;

	while (i--)
	{
		if (LINK->LINKCALL[0])
		{
			Buffer = Q_REM(&LINK->RX_Q);

			while (Buffer)
			{
				ProcessIframe(LINK, (PDATAMESSAGE)Buffer);
	
				Buffer =(PMESSAGE)Q_REM((void *)&LINK->RX_Q);
			}
		
			//	CHECK FOR OUTGOING MSGS

			if (LINK->L2STATE >= 5)				// CANT SEND TEXT TILL CONNECTED
			{
				//	CMP	VER1FLAG[EBX],1
				//	JE SHORT MAINL35			; NEED TO RETRY WITH I FRAMES IF VER 1

				//	CMP	L2RETRIES[EBX],0
				//	JNE SHORT MAINL40			; CANT SEND TEXT IF RETRYING

				if ((LINK->L2FLAGS & RNRSET) == 0)
					SDETX(LINK);
			}
		}
		LINK++;
	}

	L4BG();					// DO LEVEL 4 PROCESSING
	L3BG();

}

VOID DoListenMonitor(TRANSPORTENTRY * L4, MESSAGE * Msg)
{
	uint64_t SaveMMASK = MMASK;
	BOOL SaveMTX = MTX;
	BOOL SaveMCOM = MCOM;
	BOOL SaveMUI = MUIONLY;
	PDATAMESSAGE Buffer;
	char MonBuffer[1024];
	int len;

	UCHAR * monchars = (UCHAR *)Msg;

	if (CountFramesQueuedOnSession(L4) > 10)
		return;

	if (monchars[21] == 3 && monchars[22] == 0xcf && monchars[23] == 0xff) // Netrom Nodes
		return;

	IntSetTraceOptionsEx(L4->LISTEN, 1, 1, 0);
	
	len = IntDecodeFrame(Msg, MonBuffer, Msg->Timestamp, L4->LISTEN, FALSE, FALSE);
	
	IntSetTraceOptionsEx(SaveMMASK, SaveMTX, SaveMCOM, SaveMUI);

	if (len == 0)
		return;

	if (len > 256)
		len = 256;

	Buffer = GetBuff();

	if (Buffer)
	{
		char * ptr = &Buffer->L2DATA[0];
		Buffer->PID = 0xf0;

		memcpy(ptr, MonBuffer, len);

		Buffer->LENGTH = (USHORT)len + 4 + sizeof(void *);

		C_Q_ADD(&L4->L4TX_Q, Buffer);

		PostDataAvailable(L4);
	}
}

DllExport int APIENTRY DllBPQTRACE(MESSAGE * Msg, BOOL TOAPRS)
{
	int ret;
	GetSemaphore(&Semaphore, 88);
	ret = BPQTRACE(Msg, TOAPRS);
	FreeSemaphore(&Semaphore);
	return ret;
}

int BPQTRACE(MESSAGE * Msg, BOOL TOAPRS)
{
	//	ATTACH A COPY OF FRAME TO ANY BPQ HOST PORTS WITH MONITORING ENABLED
	
	TRANSPORTENTRY * L4 = L4TABLE;

	MESSAGE * Buffer;
	int i = BPQHOSTSTREAMS + 2;		// Include Telnet and AGW Stream

	if (TOAPRS)
		i++;						// Include APRS Stream

	while(i)
	{
		i--;

//		if (QCOUNT < 100)
//			return FALSE;

		if (BPQHOSTVECTOR[i].HOSTAPPLFLAGS & 0x80)		// Trace Enabled?
		{
			Buffer = GetBuff();
			if (Buffer)
			{
				memcpy(&Buffer->PORT, &Msg->PORT, BUFFLEN - sizeof(void *));	// Dont copy chain word
				C_Q_ADD(&BPQHOSTVECTOR[i].HOSTTRACEQ, Buffer);
			}
		}

	}

	// Also pass to any users LISTENING on this port

	i = MAXCIRCUITS;

//	if (QCOUNT < 300)
//		return FALSE;	// Until I add by session flow control		

	while (i--)
	{
		if (L4->LISTEN)
			if ((((uint64_t)1 << ((Msg->PORT & 0x7f) - 1)) & L4->LISTEN))
			//	if ((Msg->PORT & 0x7f) == L4->LISTEN)	
				DoListenMonitor(L4, Msg);
			
		L4++;
	}

	return TRUE;
}

VOID INITIALISEPORTS()
{
	char INITMSG[80];
	struct PORTCONTROL * PORT = PORTTABLE;
	
	while (PORT)
	{
		sprintf(INITMSG, "Initialising Port %02d     ", PORT->PORTNUMBER);
		WritetoConsoleLocal(INITMSG);

		PORT->PORTINITCODE(PORT);
		PORT = PORT->PORTPOINTER;
	}
}

VOID FindLostBuffers()
{
	void ** Buff;
	int n, i;
	unsigned int rev;
  void ** SaveBufferlist[NUMBEROFBUFFERS];

	UINT CodeDump[16];
	char codeText[65] = "";
	unsigned char * codeByte = (unsigned char *) CodeDump;

	PBPQVECSTRUC HOSTSESS = BPQHOSTVECTOR;
	struct _TRANSPORTENTRY * L4;	// Pointer to Session
	
	struct DEST_LIST * DEST = DESTS;

	struct ROUTE * Routes = NEIGHBOURS;
	int MaxRoutes = MAXNEIGHBOURS;
	int Queued;
	char Call[10]; 

	n = MAXDESTS;

	Debugprintf("Looking for missing Buffers");

	while (n--)
	{
		if (DEST->DEST_CALL[0] && DEST->DEST_Q)		// Spare
		{
			Debugprintf("DEST Queue %s %d", DEST->DEST_ALIAS, C_Q_COUNT(&DEST->DEST_Q));
		}

		DEST++;
	}

	n = MAXCIRCUITS;
	L4 = L4TABLE;

	while (n--)
	{
		if (L4->L4USER[0] == 0)
		{
			L4++;
			continue;
		}
		if (L4->L4TX_Q || L4->L4RX_Q || L4->L4HOLD_Q || L4->L4RESEQ_Q)
			Debugprintf("L4 %d TX %d RX %d HOLD %d RESEQ %d", MAXCIRCUITS - n, C_Q_COUNT(&L4->L4TX_Q),
				C_Q_COUNT(&L4->L4RX_Q), C_Q_COUNT(&L4->L4HOLD_Q), C_Q_COUNT(&L4->L4RESEQ_Q));
		L4++;
	}

	// Routes

	while (MaxRoutes--)
	{
		if (Routes->NEIGHBOUR_CALL[0] != 0)
		{
			Call[ConvFromAX25(Routes->NEIGHBOUR_CALL, Call)] = 0;
			if (Routes->NEIGHBOUR_LINK)					
			{
				Queued = COUNT_AT_L2(Routes->NEIGHBOUR_LINK);			// SEE HOW MANY QUEUED
				if (Queued)
					Debugprintf("Route %s %d", Call, Queued);
			}
		}
		Routes++;
	}

	// Build list of buffers, then mark off all on free Q

  memcpy(SaveBufferlist, Bufferlist, NUMBEROFBUFFERS * sizeof(void *));

	Buff = FREE_Q;

	while (Buff)
	{
		n = NUMBEROFBUFFERS;

		while (n--)
		{
			if (SaveBufferlist[n] == Buff)
			{
				SaveBufferlist[n] = 0;
				break;
			}
		}
		Buff = *Buff;
	}
	n = NUMBEROFBUFFERS;

	while (n--)
	{
		if (SaveBufferlist[n])
		{
			char * fileptr = (char *)SaveBufferlist[n];
			MESSAGE * Msg = (MESSAGE *)SaveBufferlist[n];

			memcpy(CodeDump, SaveBufferlist[n], 64);
	
			for (i = 0; i < 64; i++)
			{
				if (codeByte[i] > 0x1f && codeByte[i] < 0x80) 
					codeText[i] = codeByte[i];
				else
					codeText[i] = '.';
			}

			for (i = 0; i < 16; i++)
			{
				rev = (CodeDump[i] & 0xff) << 24;
				rev |= (CodeDump[i] & 0xff00) << 8;
				rev |= (CodeDump[i] & 0xff0000) >> 8;
				rev |= (CodeDump[i] & 0xff000000) >> 24;

				CodeDump[i] = rev;
			}

			Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
				SaveBufferlist[n], CodeDump[0], CodeDump[1], CodeDump[2], CodeDump[3], CodeDump[4], CodeDump[5], CodeDump[6], CodeDump[7]);

			Debugprintf("         %08x %08x %08x %08x %08x %08x %08x %08x %d",
				CodeDump[8], CodeDump[9], CodeDump[10], CodeDump[11], CodeDump[12], CodeDump[13], CodeDump[14], CodeDump[15], Msg->Process);
		
			Debugprintf("         %s %s", &fileptr[400], codeText);
		}
 
	}

}

void WriteConnectLog(char * fromCall, char * toCall, UCHAR * Mode)
{
	UCHAR FN[MAX_PATH];
	FILE * LogHandle;
	time_t T;
	struct tm * tm;
	char LogMsg[256];	
	int MsgLen;

	T = time(NULL);
	tm = gmtime(&T);	

	sprintf(FN,"%s/logs/ConnectLog_%02d%02d%02d.log", LogDirectory, tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday);

	LogHandle = fopen(FN, "ab");

	if (LogHandle == NULL)
		return;

	MsgLen = sprintf(LogMsg, "%02d:%02d:%02d Call from %s to %s Mode %s\r\n", tm->tm_hour, tm->tm_min, tm->tm_sec, fromCall, toCall, Mode);

	fwrite(LogMsg , 1, MsgLen, LogHandle);
	fclose(LogHandle);
}



