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



// General C Routines common to bpq32 and linbpq. Mainly moved from BPQ32.c

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma data_seg("_BPQDATA")

#include "CHeaders.h"

#include "configstructs.h"
extern struct CONFIGTABLE xxcfg;

struct TNCINFO * TNCInfo[71];		// Records are Malloc'd

extern int ReportTimer;

Dll VOID APIENTRY Send_AX(UCHAR * Block, DWORD Len, UCHAR Port);
int Check_Timer();
VOID SENDUIMESSAGE(struct DATAMESSAGE * Msg);
DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
VOID APIENTRY md5 (char *arg, unsigned char * checksum);
VOID COMSetDTR(HANDLE fd);
VOID COMClearDTR(HANDLE fd);
VOID COMSetRTS(HANDLE fd);
VOID COMClearRTS(HANDLE fd);

VOID WriteMiniDump();
void printStack(void);
char * FormatMH(PMHSTRUC MH, char Format);
void WriteConnectLog(char * fromCall, char * toCall, UCHAR * Mode);
TRANSPORTENTRY * SetupSessionFromHost(PBPQVECSTRUC HOST, UINT ApplMask);

void * picoOpenFile(char * FN);
int picoGetLine(void * file, char * Line, int maxline);
int GetNextFileBlock(void * file, char * line);
void picoCloseFile(void * file);
void picoWriteLine(void * file, char * Data, int len);
void * picoCreateFile(char * FN);
void picofprintf(void * file, const char * format, ...);


extern BOOL LogAllConnects;
extern BOOL M0LTEMap;

char * stristr (char *ch1, char *ch2);

extern VOID * ENDBUFFERPOOL;


//	Read/Write length field in a buffer header

//	Needed for Big/LittleEndian and ARM5 (unaligned operation problem) portability


VOID PutLengthinBuffer(PDATAMESSAGE buff, USHORT datalen)
{
	if (datalen <= sizeof(void *) + 4)
		datalen = sizeof(void *) + 4;		// Protect

	memcpy(&buff->LENGTH, &datalen, 2);
}

int GetLengthfromBuffer(PDATAMESSAGE buff)
{
	USHORT Length;

	memcpy(&Length, &buff->LENGTH, 2);
	return Length;
}

BOOL CheckQHeadder(UINT * Q)
{
#ifdef WIN32
	UINT Test;

	__try
	{
		Test = *Q;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Debugprintf("Invalid Q Header %p", Q);
		printStack();
		return FALSE;
	}
#endif
	return TRUE;
}

// Get buffer from Queue


VOID * _Q_REM(VOID **PQ, char * File, int Line)
{
	void ** Q;
	void ** first;
	VOID * next;
	PMESSAGE Test;

	//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

//	if (Semaphore.Flag == 0)
//		Debugprintf("Q_REM called without semaphore from %s Line %d", File, Line);

	if (CheckQHeadder((UINT *) Q) == 0)
		return(0);

	first = Q[0];

	if (first == 0)
		return (0);			// Empty

	next = first[0];			// Address of next buffer

	Q[0] = next;

	// Make sure guard zone is zeros

	Test = (PMESSAGE)first;

	if (Test->GuardZone != 0)
	{
		Debugprintf("Q_REM %p GUARD ZONE CORRUPT %x Called from %s Line %d", first, Test->GuardZone, File, Line);
		printStack();
	}

	return first;
}

// Non=pool version (for IPGateway)

VOID * _Q_REM_NP(VOID *PQ, char * File, int Line)
{
	void ** Q;
	void ** first;
	void * next;

	//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (CheckQHeadder((UINT *)Q) == 0)
		return(0);

	first = Q[0];

	if (first == 0) return (0);			// Empty

	next = first[0];			// Address of next buffer

	Q[0] = next;

	return first;
}

// Return Buffer to Free Queue

extern VOID * BUFFERPOOL;
extern void ** Bufferlist[100];
void printStack(void);

void _CheckGuardZone(char * File, int Line)
{
	int n = 0, i, offset = 0;
	PMESSAGE Test;
	UINT CodeDump[8];
	unsigned char * ptr;
}

UINT _ReleaseBuffer(VOID *pBUFF, char * File, int Line)
{
	void ** pointer, ** BUFF = pBUFF;
	int n = 0;
	void ** debug;
	PMESSAGE Test;
	UINT CodeDump[16];
	int i;
	unsigned int rev;

	pointer = FREE_Q;

	*BUFF = pointer;

	FREE_Q = BUFF;

	QCOUNT++;

	return 0;
}

int _C_Q_ADD(VOID *PQ, VOID *PBUFF, char * File, int Line)
{
	void ** Q;
	void ** BUFF = PBUFF;
	void ** next;
	PMESSAGE Test;


	int n = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	//if (Semaphore.Flag == 0)
	//	Debugprintf("C_Q_ADD called without semaphore from %s Line %d", File, Line);

	if (CheckQHeadder((UINT *)Q) == 0)			// Make sure Q header is readable
		return(0);

	// Make sure guard zone is zeros

	Test = (PMESSAGE)PBUFF;

	if (Test->GuardZone != 0)
	{
		Debugprintf("C_Q_ADD %p GUARD ZONE CORRUPT %x Called from %s Line %d", PBUFF, Test->GuardZone, File, Line);
	}

	Test = (PMESSAGE)Q;



	// Make sure address is within pool

	while (n <= NUMBEROFBUFFERS)
	{
		if (BUFF == Bufferlist[n++])
			goto BOK2;
	}

	Debugprintf("C_Q_ADD %X not in Pool called from %s Line %d", BUFF, File, Line);
	printStack();

	return 0;

BOK2:

	BUFF[0] = 0;						// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=BUFF;				// New one on front
		return(0);
	}

	next = Q[0];

	while (next[0] != 0)
	{
		next = next[0];			// Chain to end of queue
	}
	next[0] = BUFF;					// New one on end

	return(0);
}

// Non-pool version

int C_Q_ADD_NP(VOID *PQ, VOID *PBUFF)
{
	void ** Q;
	void ** BUFF = PBUFF;
	void ** next;
	int n = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (CheckQHeadder((UINT *)Q) == 0)			// Make sure Q header is readable
		return(0);

	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=BUFF;				// New one on front
//		memcpy(PQ, &BUFF, 4);
		return 0;
	}
	next = Q[0];

	while (next[0] != 0)
		next=next[0];				// Chain to end of queue

	next[0] = BUFF;					// New one on end

	return(0);
}


int C_Q_COUNT(VOID *PQ)
{
	void ** Q;
	int count = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = PQ;

	if (CheckQHeadder((UINT *)Q) == 0)			// Make sure Q header is readable
		return(0);

	//	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER

	while (*Q)
	{
		count++;
		if ((count + QCOUNT) > MAXBUFFS)
		{
 			Debugprintf("C_Q_COUNT Detected corrupt Q %p len %d", PQ, count);
			return count;
		}
		Q = *Q;
	}

	return count;
}

VOID * _GetBuff(char * File, int Line)
{
	UINT * Temp;
	MESSAGE * Msg;
	char * fptr = 0;
	unsigned char * byteaddr;

	Temp = Q_REM(&FREE_Q);

//	FindLostBuffers();

//	if (Semaphore.Flag == 0)
	//	Debugprintf("GetBuff called without semaphore from %s Line %d", File, Line);

	if (Temp)
	{
		QCOUNT--;

		if (QCOUNT < MINBUFFCOUNT)
			MINBUFFCOUNT = QCOUNT;

		Msg = (MESSAGE *)Temp;
		fptr = File + (int)strlen(File);
		while (*fptr != '\\' && *fptr != '/')
			fptr--;
		fptr++;

		// Buffer Length is BUFFLEN, but buffers are allocated 512
		// So add file info in gap between

		byteaddr = (unsigned char *)Msg;


		memset(&byteaddr[0], 0, 64);		// simplify debugging lost buffers
		memset(&byteaddr[400], 0, 64);		// simplify debugging lost buffers
		sprintf(&byteaddr[400], "%s %d", fptr, Line);

		Msg->Process = (short)GetCurrentProcessId();
		Msg->Linkptr = NULL;
		Msg->Padding[0] = 0;		// Used for modem status info 
	}
	else
		Debugprintf("Warning - Getbuff returned NULL");

	return Temp;
}

void * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr=malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr;

	if (buf == NULL) return NULL;		// Protect

	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

VOID DISPLAYCIRCUIT(TRANSPORTENTRY * L4, char * Buffer)
{
	UCHAR Type = L4->L4CIRCUITTYPE;
	struct PORTCONTROL * PORT;
	struct _LINKTABLE * LINK;
	BPQVECSTRUC * VEC;
	struct DEST_LIST * DEST;

	char Normcall[20] = "";			// Could be alias:call
	char Normcall2[11] = "";
	char Alias[11] = "";

	Buffer[0] = 0;

	switch (Type)
	{
	case PACTOR+UPLINK:

		PORT = L4->L4TARGET.PORT;

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		if (PORT)
			sprintf(Buffer, "%s %d/%d(%s)", "TNC Uplink Port", PORT->PORTNUMBER, L4->KAMSESSION, Normcall);

		return;


	case PACTOR+DOWNLINK:

		PORT = L4->L4TARGET.PORT;

		if (PORT)
			sprintf(Buffer, "%s %d/%d", "Attached to Port", PORT->PORTNUMBER, L4->KAMSESSION);
		return;


	case L2LINK+UPLINK:

		LINK = L4->L4TARGET.LINK;

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		if (LINK &&LINK->LINKPORT)
			sprintf(Buffer, "%s %d(%s)", "Uplink", LINK->LINKPORT->PORTNUMBER, Normcall);

		return;

	case L2LINK+DOWNLINK:

		LINK = L4->L4TARGET.LINK;

		if (LINK == NULL)
			return;

		ConvFromAX25(LINK->OURCALL, Normcall);
		strlop(Normcall, ' ');

		ConvFromAX25(LINK->LINKCALL, Normcall2);
		strlop(Normcall2, ' ');

		sprintf(Buffer, "%s %d(%s %s)", "Downlink", LINK->LINKPORT->PORTNUMBER, Normcall, Normcall2);
		return;

	case BPQHOST + UPLINK:
	case BPQHOST + DOWNLINK:

		// if the call has a Level 4 address display ALIAS:CALL, else just Call

		if (FindDestination(L4->L4USER, &DEST))
			Normcall[DecodeNodeName(DEST->DEST_CALL, Normcall)] = 0;		// null terminate
		else
			Normcall[ConvFromAX25(L4->L4USER, Normcall)] = 0;

		VEC = L4->L4TARGET.HOST;
		sprintf(Buffer, "%s%02d(%s)", "Host", (int)(VEC - BPQHOSTVECTOR) + 1, Normcall);
		return;

	case SESSION + DOWNLINK:
	case SESSION + UPLINK:

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		DEST = L4->L4TARGET.DEST;

		if (DEST == NULL)
			return;

		ConvFromAX25(DEST->DEST_CALL, Normcall2);
		strlop(Normcall2, ' ');

		memcpy(Alias, DEST->DEST_ALIAS, 6);
		strlop(Alias, ' ');

		sprintf(Buffer, "Circuit(%s:%s %s)", Alias, Normcall2, Normcall);

		return;
	}
}

char * Config;
static char * ptr1, * ptr2;

int GetLine(char * buf)
{
loop:

	if (ptr2 == NULL)
		return 0;

	memcpy(buf, ptr1, ptr2 - ptr1 + 2);
	buf[ptr2 - ptr1 + 2] = 0;
	ptr1 = ptr2 + 2;
	ptr2 = strchr(ptr1, 13);

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}
VOID DigiToMultiplePorts(struct PORTCONTROL * PORTVEC, PMESSAGE Msg)
{
	USHORT Mask=PORTVEC->DIGIMASK;
	int i;

	for (i=1; i<=NUMBEROFPORTS; i++)
	{
		if (Mask & 1)
		{
			// Block includes the Msg Header (7/11 bytes), Len Does not!

			Msg->PORT = i;
			Send_AX((UCHAR *)&Msg, Msg->LENGTH - MSGHDDRLEN, i);
			Mask>>=1;
		}
	}
}

int CompareAlias(struct DEST_LIST ** a, struct DEST_LIST ** b)
{
	return memcmp(a[0]->DEST_ALIAS, b[0]->DEST_ALIAS, 6); 
	/* strcmp functions works exactly as expected from comparison function */
}


int CompareNode(struct DEST_LIST ** a, struct DEST_LIST ** b)
{
	return memcmp(a[0]->DEST_CALL, b[0]->DEST_CALL, 7);
}

DllExport int APIENTRY CountFramesQueuedOnStream(int Stream)
{
	BPQVECSTRUC * PORTVEC = &BPQHOSTVECTOR[Stream-1];		// API counts from 1
	TRANSPORTENTRY * L4 = PORTVEC->HOSTSESSION;

	int Count = 0;

	if (L4)
	{
		if (L4->L4CROSSLINK)		// CONNECTED?
			Count = CountFramesQueuedOnSession(L4->L4CROSSLINK);
		else
			Count = CountFramesQueuedOnSession(L4);
	}
	return Count;
}

DllExport int APIENTRY ChangeSessionCallsign(int Stream, unsigned char * AXCall)
{
	// Equivalent to "*** linked to" command

	memcpy(BPQHOSTVECTOR[Stream-1].HOSTSESSION->L4USER, AXCall, 7);
	return (0);
}

DllExport int APIENTRY ChangeSessionPaclen(int Stream, int Paclen)
{
	BPQHOSTVECTOR[Stream-1].HOSTSESSION->SESSPACLEN = Paclen;
	return (0);
}

DllExport int APIENTRY ChangeSessionIdletime(int Stream, int idletime)
{
	if (BPQHOSTVECTOR[Stream-1].HOSTSESSION)
		BPQHOSTVECTOR[Stream-1].HOSTSESSION->L4LIMIT = idletime;
	return (0);
}

DllExport int APIENTRY Get_APPLMASK(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLMASK;
}
DllExport int APIENTRY GetStreamPID(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].STREAMOWNER;
}

DllExport int APIENTRY GetApplFlags(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLFLAGS;
}

DllExport int APIENTRY GetApplNum(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLNUM;
}

DllExport int APIENTRY GetApplMask(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTAPPLMASK;
}

DllExport BOOL APIENTRY GetAllocationState(int Stream)
{
	return	BPQHOSTVECTOR[Stream-1].HOSTFLAGS & 0x80;
}

VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);

extern int InitDone;
extern int SemHeldByAPI;
extern char pgm[256];		// Uninitialised so per process
extern int BPQHOSTAPI();


VOID POSTSTATECHANGE(BPQVECSTRUC * SESS)
{
	//	Post a message if requested

	return;
}


DllExport int APIENTRY SessionControl(int stream, int command, int Mask)
{
	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return (0);

	SESS = &BPQHOSTVECTOR[stream];

	//	Send Session Control command (BPQHOST function 6)
	//;	CL=0 CONNECT USING APPL MASK IN DL
	//;	CL=1, CONNECT. CL=2 - DISCONNECT. CL=3 RETURN TO NODE

	if 	(command > 1)
	{
		// Disconnect

		if (SESS->HOSTSESSION == 0)
		{
			SESS->HOSTFLAGS |= 1;		// State Change
			POSTSTATECHANGE(SESS);
			return 0;					// NOT CONNECTED
		}

		if (command == 3)
			SESS->HOSTFLAGS |= 0x20;	// Set Stay

		SESS->HOSTFLAGS |= 0x40;		// SET 'DISC REQ' FLAG

		return 0;
	}

	// 0 or 1 - connect

	if (SESS->HOSTSESSION)				// ALREADY CONNECTED
	{
		SESS->HOSTFLAGS |= 1;			// State Change
		POSTSTATECHANGE(SESS);
		return 0;
	}

	//	SET UP A SESSION FOR THE CONSOLE

	SESS->HOSTFLAGS |= 0x80;			// SET ALLOCATED BIT

	if (command == 1)					// Zero is mask supplied by caller
		Mask = SESS->HOSTAPPLMASK;		// SO WE GET CORRECT CALLSIGN

	L4 = SetupSessionFromHost(SESS, Mask);

	if (L4 == 0)						// tables Full
	{
		SESS->HOSTFLAGS |= 3;			// State Change
		POSTSTATECHANGE(SESS);
		return 0;
	}

	SESS->HOSTSESSION = L4;
	L4->L4CIRCUITTYPE = BPQHOST | UPLINK;
 	L4->Secure_Session = AuthorisedProgram;	// Secure Host Session

	SESS->HOSTFLAGS |= 1;		// State Change
	POSTSTATECHANGE(SESS);
	return 0;					// ALREADY CONNECTED
}

int FindFreeStreamEx(int GetSem);

int FindFreeStreamNoSem()
{
	return FindFreeStreamEx(0);
}

DllExport int APIENTRY FindFreeStream()
{
	return FindFreeStreamEx(1);
}

int FindFreeStreamEx(int GetSem)
{
	int stream, n;
	BPQVECSTRUC * PORTVEC;

//	Returns number of first unused BPQHOST stream. If none available,
//	returns 255. See API function 13.

	// if init has not yet been run, wait.

	while (InitDone == 0)
	{
		Debugprintf("Waiting for init to complete");
		Sleep(1000);
	}

	if (InitDone == -1)			// Init failed
		exit(0);

	if (GetSem)
		GetSemaphore(&Semaphore, 9);

	stream = 0;
	n = 64;

	while (n--)
	{
		PORTVEC = &BPQHOSTVECTOR[stream++];
		if ((PORTVEC->HOSTFLAGS & 0x80) == 0)
		{
			PORTVEC->STREAMOWNER=GetCurrentProcessId();
			PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
			memcpy(&PORTVEC->PgmName[0], pgm, 31);
			if (GetSem)
				FreeSemaphore(&Semaphore);
			return stream;
		}
	}

	if (GetSem)
		FreeSemaphore(&Semaphore);

	return 255;
}

DllExport int APIENTRY AllocateStream(int stream)
{
//	Allocate stream. If stream is already allocated, return nonzero.
//	Otherwise allocate stream, and return zero.

	BPQVECSTRUC * PORTVEC = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	if ((PORTVEC->HOSTFLAGS & 0x80) == 0)
	{
		PORTVEC->STREAMOWNER=GetCurrentProcessId();
		PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
		memcpy(&PORTVEC->PgmName[0], pgm, 31);
		FreeSemaphore(&Semaphore);
		return 0;
	}

	return 1;				// Already allocated
}


DllExport int APIENTRY DeallocateStream(int stream)
{
	BPQVECSTRUC * PORTVEC;
	UINT * monbuff;
	BOOL GotSem = Semaphore.Flag;

//	Release stream.

	stream--;

	if (stream < 0 || stream > 63)
		return (0);

	PORTVEC=&BPQHOSTVECTOR[stream];

	PORTVEC->STREAMOWNER=0;
	PORTVEC->PgmName[0] = 0;
	PORTVEC->HOSTAPPLFLAGS=0;
	PORTVEC->HOSTAPPLMASK=0;
	

	// Clear Trace Queue

	if (PORTVEC->HOSTSESSION)
		SessionControl(stream + 1, 2, 0);

	if (GotSem == 0)
		GetSemaphore(&Semaphore, 0);

	while (PORTVEC->HOSTTRACEQ)
	{
		monbuff = Q_REM((void *)&PORTVEC->HOSTTRACEQ);
		ReleaseBuffer(monbuff);
	}

	if (GotSem == 0)
		FreeSemaphore(&Semaphore);

	PORTVEC->HOSTFLAGS &= 0x60;			// Clear Allocated. Must leave any DISC Pending bits

	return(0);
}
DllExport int APIENTRY SessionState(int stream, int * state, int * change)
{
	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.

	BPQVECSTRUC * HOST = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	Check_Timer();				// In case Appl doesnt call it often ehough

	GetSemaphore(&Semaphore, 20);

	//	CX = 0 if stream disconnected or CX = 1 if stream connected
	//	DX = 0 if no change of state since last read, or DX = 1 if
	//	       the connected/disconnected state has changed since
	//	       last read (ie. delta-stream status).

	//	HOSTFLAGS = Bit 80 = Allocated
	//		  Bit 40 = Disc Request
	//		  Bit 20 = Stay Flag
	//		  Bit 02 and 01 State Change Bits

	if ((HOST->HOSTFLAGS & 3) == 0)
		// No Chaange
		*change = 0;
	else
		*change = 1;

	if (HOST->HOSTSESSION)			// LOCAL SESSION
		// Connected
		*state = 1;
	else
		*state = 0;

	HOST->HOSTFLAGS &= 0xFC;		// Clear Change Bitd

	FreeSemaphore(&Semaphore);
	return 0;
}

DllExport int APIENTRY SessionStateNoAck(int stream, int * state)
{
	//	Get current Session State. Dont ACK any change
	//	See BPQHOST function 4

	BPQVECSTRUC * HOST = &BPQHOSTVECTOR[stream -1];		// API counts from 1

	Check_Timer();				// In case Appl doesnt call it often ehough

	if (HOST->HOSTSESSION)			// LOCAL SESSION
		// Connected
		*state = 1;
	else
		*state = 0;

	return 0;
}

int SendMsg(int stream, char * msg, int len)
{
	//	Send message to stream (BPQHOST Function 2)

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	PDATAMESSAGE MSG;

	Check_Timer();

	if (len > 256)
		return 0;						// IGNORE

	if (stream == 0)
	{
		// Send UNPROTO - SEND FRAME TO ALL RADIO PORTS

		//	COPY DATA TO A BUFFER IN OUR SEGMENTS - SIMPLFIES THINGS LATER

		if (QCOUNT < 15)
			return 0;					// Dont want to run out

		GetSemaphore(&Semaphore, 10);

		if ((MSG = GetBuff()) == 0)
		{
			FreeSemaphore(&Semaphore);
			return 0;
		}

		MSG->PID = 0xF0;				// Normal Data PID

		memcpy(&MSG->L2DATA[0], msg, len);
		MSG->LENGTH = (len + MSGHDDRLEN + 1);

		SENDUIMESSAGE(MSG);
		ReleaseBuffer(MSG);
		FreeSemaphore(&Semaphore);
		return 0;
	}

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;

	GetSemaphore(&Semaphore, 22);

	SESS->HOSTFLAGS |= 0x80;		// SET ALLOCATED BIT

	if (QCOUNT < 15)				// PLENTY FREE?
	{
		FreeSemaphore(&Semaphore);
		return 1;
	}

	// Dont allow massive queues to form

	if (QCOUNT < 15)
	{
		int n = CountFramesQueuedOnStream(stream + 1);

		if (n > 10)
		{
			Debugprintf("Stream %d QCOUNT %d Q Len %d - discarding", stream, QCOUNT, n);
			FreeSemaphore(&Semaphore);
			return 1;
		}
	}

	if ((MSG = GetBuff()) == 0)
	{
		FreeSemaphore(&Semaphore);
		return 1;
	}

	MSG->PID = 0xF0;				// Normal Data PID

	memcpy(&MSG->L2DATA[0], msg, len);
	MSG->LENGTH = len + MSGHDDRLEN + 1;

	//	IF CONNECTED, PASS MESSAGE TO TARGET CIRCUIT - FLOW CONTROL AND
	//	DELAYED DISC ONLY WORK ON ONE SIDE

	Partner = L4->L4CROSSLINK;

	L4->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT

	if (Partner && Partner->L4STATE > 4)	// Partner and link up
	{
		//	Connected

		Partner->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT
		C_Q_ADD(&Partner->L4TX_Q, MSG);
		PostDataAvailable(Partner);
	}
	else
		C_Q_ADD(&L4->L4RX_Q, MSG);

	FreeSemaphore(&Semaphore);
	return 0;
}
DllExport int APIENTRY SendRaw(int port, char * msg, int len)
{
	struct PORTCONTROL * PORT;
	MESSAGE * MSG;

	Check_Timer();

	//	Send Raw (KISS mode) frame to port (BPQHOST function 10)

	if (len > (MAXDATA - (MSGHDDRLEN + 8)))
		return 0;

	if (QCOUNT < 15)
		return 1;

	//	GET A BUFFER

	PORT = GetPortTableEntryFromSlot(port);

	if (PORT == 0)
		return 0;

	GetSemaphore(&Semaphore, 24);

	MSG = GetBuff();

	if (MSG == 0)
	{
		FreeSemaphore(&Semaphore);
		return 1;
	}

	memcpy(MSG->DEST, msg, len);

	MSG->LENGTH = len + MSGHDDRLEN;

	if (PORT->PROTOCOL == 10)		 // PACTOR/WINMOR Style
	{
		//	Pactor Style. Probably will only be used for Tracker uneless we do APRS over V4 or WINMOR

		EXTPORTDATA * EXTPORT = (EXTPORTDATA *) PORT;

		C_Q_ADD(&EXTPORT->UI_Q,	MSG);

		FreeSemaphore(&Semaphore);
		return 0;
	}

	MSG->PORT = PORT->PORTNUMBER;

	PUT_ON_PORT_Q(PORT, MSG);

	FreeSemaphore(&Semaphore);
	return 0;
}

DllExport time_t APIENTRY GetRaw(int stream, char * msg, int * len, int * count)
{
	time_t Stamp;
	BPQVECSTRUC * SESS;
	PMESSAGE MSG;
	int Msglen;

	Check_Timer();

	*len = 0;
	*count = 0;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];

	GetSemaphore(&Semaphore, 26);

	if (SESS->HOSTTRACEQ == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	MSG = Q_REM((void *)&SESS->HOSTTRACEQ);

	Msglen = MSG->LENGTH;

	if (Msglen < 0 || Msglen > 350)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	Stamp = MSG->Timestamp;

	memcpy(msg, MSG, BUFFLEN - sizeof(void *));		// To c

	*len = Msglen;

	ReleaseBuffer(MSG);

	*count = C_Q_COUNT(&SESS->HOSTTRACEQ);
	FreeSemaphore(&Semaphore);

	return Stamp;
}

DllExport int APIENTRY GetMsg(int stream, char * msg, int * len, int * count )
{
//	Get message from stream. Returns length, and count of frames
//	still waiting to be collected. (BPQHOST function 3)
//	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
//		in CX.  BX returns the number of outstanding frames still to
//		be received (ie. after this one) or zero if no more frames
//		(ie. this is last one).
//

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	PDATAMESSAGE MSG;
	int Msglen;

	Check_Timer();

	*len = 0;
	*count = 0;

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;


	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore(&Semaphore, 25);

	if (L4 == 0 || L4->L4TX_Q == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	L4->L4KILLTIMER = 0;		// RESET SESSION TIMEOUT

	if(L4->L4CROSSLINK)
		L4->L4CROSSLINK->L4KILLTIMER = 0;

	MSG = Q_REM((void *)&L4->L4TX_Q);

	Msglen = MSG->LENGTH - (MSGHDDRLEN + 1);	// Dont want PID

	if (Msglen < 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	if (Msglen > 256)
		Msglen = 256;

	memcpy(msg, &MSG->L2DATA[0], Msglen);

	*len = Msglen;

	ReleaseBuffer(MSG);

	*count = C_Q_COUNT(&L4->L4TX_Q);
	FreeSemaphore(&Semaphore);

	return 0;
}


DllExport int APIENTRY RXCount(int stream)
{
//	Returns count of packets waiting on stream
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;			// NOT CONNECTED

	return C_Q_COUNT(&L4->L4TX_Q);
}

DllExport int APIENTRY TXCount(int stream)
{
//	Returns number of packets on TX queue for stream
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	if (L4 == 0)
		return 0;			// NOT CONNECTED

	L4 = L4->L4CROSSLINK;

	if (L4 == 0)
		return 0;			// NOTHING ro Q on

	return (CountFramesQueuedOnSession(L4));
}

DllExport int APIENTRY MONCount(int stream)
{
//	Returns number of monitor frames available
//	 (BPQHOST function 7 (part)).

	BPQVECSTRUC * SESS;

	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];

	return C_Q_COUNT(&SESS->HOSTTRACEQ);
}


DllExport int APIENTRY GetCallsign(int stream, char * callsign)
{
	//	Returns call connected on stream (BPQHOST function 8 (part)).

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	UCHAR  Call[11] = "SWITCH    ";
	UCHAR * AXCall = NULL;
	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore(&Semaphore, 26);

	if (L4 == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	Partner = L4->L4CROSSLINK;

	if (Partner)
	{
		//	CONNECTED OUT - GET TARGET SESSION

		if (Partner->L4CIRCUITTYPE & BPQHOST)
		{
			AXCall = &Partner->L4USER[0];
		}
		else if (Partner->L4CIRCUITTYPE & L2LINK)
		{
			struct _LINKTABLE * LINK = Partner->L4TARGET.LINK;

			if (LINK)
				AXCall = LINK->LINKCALL;

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				// IF UPLINK, SHOULD USE SESSION CALL, IN CASE *** LINKED HAS BEEN USED

				AXCall = &Partner->L4USER[0];
			}
		}
		else if (Partner->L4CIRCUITTYPE & PACTOR)
		{
			//	PACTOR Type - Frames are queued on the Port Entry

			EXTPORTDATA * EXTPORT = Partner->L4TARGET.EXTPORT;

			if (EXTPORT)
				AXCall = &EXTPORT->ATTACHEDSESSIONS[Partner->KAMSESSION]->L4USER[0];

		}
		else
		{
			//	MUST BE NODE SESSION

			//	ANOTHER NODE

			//	IF THE HOST IS THE UPLINKING STATION, WE NEED THE TARGET CALL

			if (L4->L4CIRCUITTYPE & UPLINK)
			{
				struct DEST_LIST *DEST = Partner->L4TARGET.DEST;

				if (DEST)
					AXCall = &DEST->DEST_CALL[0];
			}
			else
				AXCall = Partner->L4USER;
		}
		if (AXCall)
			ConvFromAX25(AXCall, Call);
	}

	memcpy(callsign, Call, 10);

	FreeSemaphore(&Semaphore);
	return 0;
}

DllExport int APIENTRY GetConnectionInfo(int stream, char * callsign,
										 int * port, int * sesstype, int * paclen,
										 int * maxframe, int * l4window)
{
	// Return the Secure Session Flag rather than not connected

	BPQVECSTRUC * SESS;
	TRANSPORTENTRY * L4;
	TRANSPORTENTRY * Partner;
	UCHAR  Call[11] = "SWITCH    ";
	UCHAR * AXCall;
	Check_Timer();

	stream--;						// API uses 1 - 64

	if (stream < 0 || stream > 63)
		return 0;

	SESS = &BPQHOSTVECTOR[stream];
	L4 = SESS->HOSTSESSION;

	GetSemaphore(&Semaphore, 27);

	if (L4 == 0)
	{
		FreeSemaphore(&Semaphore);
		return 0;
	}

	Partner = L4->L4CROSSLINK;

	// Return the Secure Session Flag rather than not connected

	//		AL = Radio port on which channel is connected (or zero)
	//		AH = SESSION TYPE BITS
	//		EBX = L2 paclen for the radio port
	//		ECX = L2 maxframe for the radio port
	//		EDX = L4 window size (if L4 circuit, or zero) or -1 if not connected
	//		ES:DI = CALLSIGN

	*port = 0;
	*sesstype = 0;
	*paclen = 0;
	*maxframe = 0;
	*l4window = 0;
	if (L4->SESSPACLEN)
		*paclen = L4->SESSPACLEN;
	else
		*paclen = 256;

	if (Partner)
	{
		//	CONNECTED OUT - GET TARGET SESSION

		*l4window = Partner->L4WINDOW;
		*sesstype = Partner->L4CIRCUITTYPE;

		if (Partner->L4CIRCUITTYPE & BPQHOST)
		{
			AXCall = &Partner->L4USER[0];
		}
		else if (Partner->L4CIRCUITTYPE & L2LINK)
		{
			struct _LINKTABLE * LINK = Partner->L4TARGET.LINK;

			//	EXTRACT PORT AND MAXFRAME

			*port = LINK->LINKPORT->PORTNUMBER;
			*maxframe = LINK->LINKWINDOW;
			*l4window = 0;

			AXCall = LINK->LINKCALL;

			if (Partner->L4CIRCUITTYPE & UPLINK)
			{
				// IF UPLINK, SHOULD USE SESSION CALL, IN CASE *** LINKED HAS BEEN USED

				AXCall = &Partner->L4USER[0];
			}
		}
		else if (Partner->L4CIRCUITTYPE & PACTOR)
		{
			//	PACTOR Type - Frames are queued on the Port Entry

			EXTPORTDATA * EXTPORT = Partner->L4TARGET.EXTPORT;

			*port = EXTPORT->PORTCONTROL.PORTNUMBER;
			AXCall = &EXTPORT->ATTACHEDSESSIONS[Partner->KAMSESSION]->L4USER[0];

		}
		else
		{
			//	MUST BE NODE SESSION

			//	ANOTHER NODE

			//	IF THE HOST IS THE UPLINKING STATION, WE NEED THE TARGET CALL

			if (L4->L4CIRCUITTYPE & UPLINK)
			{
				struct DEST_LIST *DEST = Partner->L4TARGET.DEST;

				AXCall = &DEST->DEST_CALL[0];
			}
			else
				AXCall = Partner->L4USER;
		}
		ConvFromAX25(AXCall, Call);
	}

	memcpy(callsign, Call, 10);

	FreeSemaphore(&Semaphore);

	if (Partner)
		return Partner->Secure_Session;

	return 0;
}


DllExport int APIENTRY SetAppl(int stream, int flags, int mask)
{
//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in EDX (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.


	BPQVECSTRUC * PORTVEC;
	stream--;

	if (stream < 0 || stream > 63)
		return (0);

	PORTVEC=&BPQHOSTVECTOR[stream];

	PORTVEC->HOSTAPPLFLAGS = flags;
	PORTVEC->HOSTAPPLMASK = mask;

	// If either is non-zero, set allocated and Process. This gets round problem with
	// stations that don't call allocate stream

	if (flags || mask)
	{
		if ((PORTVEC->HOSTFLAGS & 128) == 0)	// Not allocated
		{
			PORTVEC->STREAMOWNER=GetCurrentProcessId();
			memcpy(&PORTVEC->PgmName[0], pgm, 31);
			PORTVEC->HOSTFLAGS = 128;				 // SET ALLOCATED BIT, clear others
		}
	}

	return (0);
}

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntry(int portslot)		// Kept for Legacy apps
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

// Proc below renamed to avoid confusion with GetPortTableEntryFromPortNum

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

int CanPortDigi(int Port)
{
	struct PORTCONTROL * PORTVEC = GetPortTableEntryFromPortNum(Port);
	struct TNCINFO * TNC;

	if (PORTVEC == NULL)
		return FALSE;

	TNC = PORTVEC->TNC;

	if (TNC == NULL)
		return TRUE;

	return TRUE;
}

struct PORTCONTROL * APIENTRY GetPortTableEntryFromPortNum(int portnum)
{
	struct PORTCONTROL * PORTVEC = PORTTABLE;

	do
	{
		if (PORTVEC->PORTNUMBER == portnum)
			return PORTVEC;

		PORTVEC=PORTVEC->PORTPOINTER;
	}
	while (PORTVEC);

	return NULL;
}

DllExport UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	memcpy(Desc, PORTVEC->PORTDESCRIPTION, 30);
	Desc[30]=0;

	return 0;
}

// Standard serial port handling routines, used by lots of modules.

int picoOpenSerial(char * Name, int speed);
int picoWriteSerial(int Chan, char * Name, int speed);
int picoReadSerial(int Chan, char  * Name, int speed);




int OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char Port[256];
	char buf[100];
  int fd;

	fd = picoOpenSerial(pPort, speed);


	return fd;
}

int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error);

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	BOOL Error;
	return 0;
}

// version to pass read error back to caller

int ReadCOMBlockEx(HANDLE fd, char * Block, int MaxLength, BOOL * Error)
{
	int Length = 0;

	return Length;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	//	Some systems seem to have a very small max write size
	
	int ToSend = BytesToWrite;
	int Sent = 0, ret;
	int loops = 100;

	while (ToSend && loops-- > 0)
	{
		ret = write(fd, &Block[Sent], ToSend);

		if (ret >= ToSend)
			return TRUE;

		if (ret == -1)
		{
			if (errno != 11 && errno != 35)					// Would Block
				return FALSE;
	
			Sleep(10);
			ret = 0;
		}
						
		Sent += ret;
		ToSend -= ret;
	}

//	if (ToSend)
//	{
//		// Send timed out. Close and reopen device
//
//	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	if (fd == 0)
		return;

	close(fd);
	fd = 0;
}

VOID COMSetDTR(HANDLE fd)
{


}

VOID COMClearDTR(HANDLE fd)
{

}

VOID COMSetRTS(HANDLE fd)
{

}

VOID COMClearRTS(HANDLE fd)
{

}


int MaxNodes;
int MaxRoutes;
int NodeLen;
int RouteLen;
struct DEST_LIST * Dests;
struct ROUTE * Routes;


void writeNodesLine(char * line);

static FILE * file;

int DoRoutes()
{
	char digis[30] = "";
	int count, len;
	char Normcall[10], Portcall[10];
	char line[80];

  Routes = NEIGHBOURS;
	RouteLen = ROUTE_LEN;
	MaxRoutes = MAXNEIGHBOURS;

	for (count=0; count<MaxRoutes; count++)
	{
		if (Routes->NEIGHBOUR_CALL[0] != 0)
		{
			len=ConvFromAX25(Routes->NEIGHBOUR_CALL,Normcall);
			Normcall[len]=0;

			if (Routes->NEIGHBOUR_DIGI1[0] != 0)
			{
				memcpy(digis," VIA ",5);

				len=ConvFromAX25(Routes->NEIGHBOUR_DIGI1,Portcall);
				Portcall[len]=0;
				strcpy(&digis[5],Portcall);

				if (Routes->NEIGHBOUR_DIGI2[0] != 0)
				{
					len=ConvFromAX25(Routes->NEIGHBOUR_DIGI2,Portcall);
					Portcall[len]=0;
					strcat(digis," ");
					strcat(digis,Portcall);
				}
			}
			else
				digis[0] = 0;

			len=sprintf(line,
					"ROUTE ADD %s %d %d %s %d %d %d %d %d\n",
					Normcall,
					Routes->NEIGHBOUR_PORT,
					Routes->NEIGHBOUR_QUAL, digis,
					Routes->NBOUR_MAXFRAME,
					Routes->NBOUR_FRACK,
					Routes->NBOUR_PACLEN,
					Routes->INP3Node | (Routes->NoKeepAlive << 2),
					Routes->OtherendsRouteQual);

					writeNodesLine(line);
		}

		Routes+=1;
	}

	return (0);
}

int DoNodes()
{
	int count, len, cursor, i;
	char Normcall[10], Portcall[10];
	char line[80];
	char Alias[7];

	Dests = DESTS;
	NodeLen = DEST_LIST_LEN;
	MaxNodes = MAXDESTS;

	Dests-=1;

	for (count=0; count<MaxNodes; count++)
	{
		Dests+=1;

		if (Dests->NRROUTE[0].ROUT_NEIGHBOUR == 0)
			continue;

		{
			len=ConvFromAX25(Dests->DEST_CALL,Normcall);
			Normcall[len]=0;

			memcpy(Alias,Dests->DEST_ALIAS,6);

			Alias[6]=0;

			for (i=0;i<6;i++)
			{
				if (Alias[i] == ' ')
					Alias[i] = 0;
			}

			cursor=sprintf(line,"NODE ADD %s:%s ", Alias,Normcall);

			if (Dests->NRROUTE[0].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[0].ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=sprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE[0].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE[0].ROUT_QUALITY);

				cursor+=len;

				if (Dests->NRROUTE[0].ROUT_OBSCOUNT > 127)
				{
					len=sprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

			if (Dests->NRROUTE[1].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[1].ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=sprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE[1].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE[1].ROUT_QUALITY);

				cursor+=len;

				if (Dests->NRROUTE[1].ROUT_OBSCOUNT > 127)
				{
					len=sprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

		if (Dests->NRROUTE[2].ROUT_NEIGHBOUR != 0 && Dests->NRROUTE[2].ROUT_NEIGHBOUR->INP3Node == 0)
		{
			len=ConvFromAX25(
				Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len]=0;

			len=sprintf(&line[cursor],"%s %d %d ",
				Portcall,
				Dests->NRROUTE[2].ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
				Dests->NRROUTE[2].ROUT_QUALITY);

			cursor+=len;

			if (Dests->NRROUTE[2].ROUT_OBSCOUNT > 127)
			{
				len=sprintf(&line[cursor],"! ");
				cursor+=len;
			}
		}

		if (cursor > 30)
		{
			line[cursor++]='\n';
			line[cursor++]=0;
			writeNodesLine(line);
		}
		}
	}
	return (0);
}

void SaveMH()
{
	struct PORTCONTROL * PORT = PORTTABLE;
	void *file;
	
	file = picoCreateFile("MHSave.txt");
	
	while (PORT)
	{	
		int Port = 0;
		char * ptr;
	
		MHSTRUC * MH = PORT->PORTMHEARD;

		int count = MHENTRIES;
		int n;
		char Normcall[20];
		char From[10];
		char DigiList[100];
		char * Output;
		int len;
		char Digi = 0;


		// Note that the MHDIGIS field may contain rubbish. You have to check End of Address bit to find
		// how many digis there are
	
		if (MH == NULL)
			continue;
		
		picofprintf(file, "Port:%d\n", PORT->PORTNUMBER);
	
		while (count--)
		{
			if (MH->MHCALL[0] == 0)
				break;

			Digi = 0;
		
			len = ConvFromAX25(MH->MHCALL, Normcall);
			Normcall[len] = 0;

			n = 8;					// Max number of digi-peaters

			ptr = &MH->MHCALL[6];	// End of Address bit

			Output = &DigiList[0];

			if ((*ptr & 1) == 0)
			{
				// at least one digi

				strcpy(Output, "via ");
				Output += 4;
		
				while ((*ptr & 1) == 0)
				{
					//	MORE TO COME
	
					From[ConvFromAX25(ptr + 1, From)] = 0;
					Output += sprintf((char *)Output, "%s", From);
	
					ptr += 7;
					n--;

					if (n == 0)
						break;

					// See if digi actioned - put a * on last actioned

					if (*ptr & 0x80)
					{
						if (*ptr & 1)						// if last address, must need *
						{
							*(Output++) = '*';
							Digi = '*';
						}

						else
							if ((ptr[7] & 0x80) == 0)		// Repeased by next?
							{
								*(Output++) = '*';			// No, so need *
								Digi = '*';
							}
					

					}
					*(Output++) = ',';
				}		
				*(--Output) = 0;							// remove last comma
			}
			else 
				*(Output) = 0;

			// if we used a digi set * on call and display via string


			if (Digi)
				Normcall[len++] = Digi;
			else
				DigiList[0] = 0;	// Dont show list if not used

			Normcall[len++] = 0;

			ptr = FormatMH(MH, 'U');

			ptr[15] = 0;
		
			if (MH->MHDIGI)
				picofprintf(file, "%d %6d %-10s%c %s %s|%s|%s\n", (int)MH->MHTIME, MH->MHCOUNT, Normcall, MH->MHDIGI, ptr, DigiList, MH->MHLocator, MH->MHFreq);
			else
				picofprintf(file, "%d %6d %-10s%c %s %s|%s|%s\n", (int)MH->MHTIME, MH->MHCOUNT, Normcall, ' ', ptr, DigiList, MH->MHLocator, MH->MHFreq);

			MH++;
		}
		PORT = PORT->PORTPOINTER;
	}

	picoCloseFile(file);

	return;
}



DllExport int APIENTRY ClearNodes ()
{
	char FN[250];

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"/");
		strcat(FN,"BPQNODES.dat");
	}

	if ((file = fopen(FN, "w")) == NULL)
		return FALSE;

	fclose(file);

	return (0);
}
char * FormatUptime(int Uptime)
 {
	struct tm * TM;
	static char UPTime[50];
	time_t szClock = Uptime * 60;

	TM = gmtime(&szClock);

	sprintf(UPTime, "Uptime (Days Hours Mins)     %.2d:%.2d:%.2d\r",
		TM->tm_yday, TM->tm_hour, TM->tm_min);

	return UPTime;
 }

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


char * FormatMH(PMHSTRUC MH, char Format)
{
	struct tm * TM;
	static char MHTime[50];
	time_t szClock;
	char LOC[7];

	memcpy(LOC, MH->MHLocator, 6);
	LOC[6] = 0;

	if (Format == 'U' || Format =='L')
		szClock = MH->MHTIME;
	else
		szClock = time(NULL) - MH->MHTIME;

	if (Format == 'L')
		TM = localtime(&szClock);
	else
		TM = gmtime(&szClock);

	if (Format == 'U' || Format =='L')
		sprintf(MHTime, "%s %02d %.2d:%.2d:%.2d  %s %s",
			month[TM->tm_mon], TM->tm_mday, TM->tm_hour, TM->tm_min, TM->tm_sec, MH->MHFreq, LOC);
	else
		sprintf(MHTime, "%.2d:%.2d:%.2d:%.2d  %s %s",
			TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, MH->MHFreq, LOC);

	return MHTime;

}


Dll VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset)
{
	// Create a time dependent One Time Password from the KeyPhrase
	// TimeOffset is used when checking to allow for slight variation in clocks

	time_t NOW = time(NULL);
	UCHAR Hash[16];
	char Key[1000];
	int i, chr;

	NOW = NOW/30 + TimeOffset;				// Only Change every 30 secs

	sprintf(Key, "%s%x", KeyPhrase, (int)NOW);

	md5(Key, Hash);

	for (i=0; i<16; i++)
	{
		chr = (Hash[i] & 31);
		if (chr > 9) chr += 7;

		Password[i] = chr + 48;
	}

	Password[16] = 0;
	return;
}

Dll BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase)
{
	char CheckPassword[17];
	int Offsets[10] = {0, -1, 1, -2, 2, -3, 3, -4, 4};
	int i, Pass;

	if (strlen(Password) < 16)
		Pass = atoi(Password);

	for (i = 0; i < 9; i++)
	{
		CreateOneTimePassword(CheckPassword, KeyPhrase, Offsets[i]);

		if (strlen(Password) < 16)
		{
			// Using a numeric extract

			long long Val;

			memcpy(&Val, CheckPassword, 8);
			Val = Val %= 1000000;

			if (Pass == Val)
				return TRUE;
		}
		else
			if (memcmp(Password, CheckPassword, 16) == 0)
				return TRUE;
	}

	return FALSE;
}


DllExport BOOL ConvToAX25Ex(unsigned char * callsign, unsigned char * ax25call)
{
	// Allows SSID's of 'T and 'R'
	
	int i;

	memset(ax25call,0x40,6);		// in case short
	ax25call[6]=0x60;				// default SSID

	for (i=0;i<7;i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			
			if (callsign[i+1] == 'T')
			{
				ax25call[6]=0x42;
				return TRUE;
			}

			if (callsign[i+1] == 'R')
			{
				ax25call[6]=0x44;
				return TRUE;
			}
			i = atoi(&callsign[i+1]);

			if (i < 16)
			{
				ax25call[6] |= i<<1;
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == 13 || callsign[i] == ' ' || callsign[i] == ',')
		{
			//
			//	End of call - no ssid
			//
			return (TRUE);
		}

		ax25call[i] = callsign[i] << 1;
	}

	//
	//	Too many chars
	//

	return (FALSE);
}


DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call)
{
	int i;

	memset(ax25call,0x40,6);		// in case short
	ax25call[6]=0x60;				// default SSID

	for (i=0;i<7;i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			i = atoi(&callsign[i+1]);

			if (i < 16)
			{
				ax25call[6] |= i<<1;
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == 13 || callsign[i] == ' ' || callsign[i] == ',')
		{
			//
			//	End of call - no ssid
			//
			return (TRUE);
		}

		ax25call[i] = callsign[i] << 1;
	}

	//
	//	Too many chars
	//

	return (FALSE);
}


DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall)
{
	int in,out=0;
	unsigned char chr;

	memset(outcall,0x20,10);

	for (in=0;in<6;in++)
	{
		chr=incall[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++]=chr;
	}

	chr=incall[6];				// ssid

	if (chr == 0x42)
	{
		outcall[out++]='-';
		outcall[out++]='T';
		return out;
	}

	if (chr == 0x44)
	{
		outcall[out++]='-';
		outcall[out++]='R';
		return out;
	}

	chr >>= 1;
	chr	&= 15;

	if (chr > 0)
	{
		outcall[out++]='-';
		if (chr > 9)
		{
			chr-=10;
			outcall[out++]='1';
		}
		chr+=48;
		outcall[out++]=chr;
	}
	return (out);
}

unsigned short int compute_crc(unsigned char *buf, int txlen);


extern char LOCATOR[];			// Locator for Reporting - may be Maidenhead or LAT:LON
extern char MAPCOMMENT[];		// Locator for Reporting - may be Maidenhead or LAT:LON
extern char LOC[7];				// Maidenhead Locator for Reporting
extern char ReportDest[7];


VOID SendReportMsg(char * buff, int txlen)
{}
VOID SendLocation()
{
}




VOID SendMH(struct TNCINFO * TNC, char * call, char * freq, char * LOC, char * Mode)
{

}

time_t TimeLastNRRouteSent = 0;

char NRRouteMessage[256];
int NRRouteLen = 0;


VOID SendNETROMRoute(struct PORTCONTROL * PORT, unsigned char * axcall)
{
	//	Called to update Link Map when a NODES Broadcast is received
	//  Batch to reduce Load

	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[300];
	int Len;
	char Call[10];
	char Report[16];
	time_t Now = time(NULL);
	int NeedSend = FALSE;


	Call[ConvFromAX25(axcall, Call)] = 0;

	sprintf(Report, "%s,%d,", Call, PORT->PORTTYPE);

	if (Now - TimeLastNRRouteSent > 60)
		NeedSend = TRUE;
	
	if (strstr(NRRouteMessage, Report) == 0)	//  reported recently
		strcat(NRRouteMessage, Report);
		
	if (strlen(NRRouteMessage) > 230 || NeedSend)
	{
		Len = sprintf(Msg, "LINK %s", NRRouteMessage);

		// Block includes the Msg Header (7 bytes), Len Does not!

		memcpy(AXPTR->DEST, ReportDest, 7);
		memcpy(AXPTR->ORIGIN, MYCALL, 7);
		AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
		AXPTR->DEST[6] |= 0x80;			// set Command Bit

		AXPTR->ORIGIN[6] |= 1;			// Set End of Call
		AXPTR->CTL = 3;		//UI
		AXPTR->PID = 0xf0;
		memcpy(AXPTR->L2DATA, Msg, Len);

		SendReportMsg((char *)&AXMSG.DEST, Len + 16) ;

		TimeLastNRRouteSent = Now;
		NRRouteMessage[0] = 0;
	}

	return;

}

DllExport char * APIENTRY GetApplCall(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLCALL_TEXT);
}
DllExport char * APIENTRY GetApplAlias(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLALIAS_TEXT);
}

DllExport int32_t APIENTRY GetApplQual(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return 0;

	return (APPLCALLTABLE[Appl-1].APPLQUAL);
}

char * GetApplCallFromName(char * App)
{
	int i;
	char PaddedAppl[13] = "            ";

	memcpy(PaddedAppl, App, (int)strlen(App));

	for (i = 0; i < NumberofAppls; i++)
	{
		if (memcmp(&APPLCALLTABLE[i].APPLCMD, PaddedAppl, 12) == 0)
			return &APPLCALLTABLE[i].APPLCALL_TEXT[0];
	}
	return NULL;
}


DllExport char * APIENTRY GetApplName(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLCMD);
}

DllExport int APIENTRY GetNumberofPorts()
{
	return (NUMBEROFPORTS);
}

DllExport int APIENTRY GetPortNumber(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC->PORTNUMBER;

}

DllExport char * APIENTRY GetVersionString()
{
//	return ((char *)&VersionStringWithBuild);
	return ((char *)&VersionString);
}



void GetSemaphore(struct SEM * Semaphore, int ID)
{
	//
	//	Wait for it to be free
	//

	if (Semaphore->Flag != 0)
	{
		Semaphore->Clashes++;
	}

loop1:

	while (Semaphore->Flag != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

	//if (__sync_lock_test_and_set(&Semaphore->Flag, 1) != 0)

		// Failed to get it
	//	goto loop1;		// try again;


	//Ok. got it

	Semaphore->Gets++;
	Semaphore->SemProcessID = GetCurrentProcessId();
	Semaphore->SemThreadID = GetCurrentThreadId();
	SemHeldByAPI = ID;

	return;
}

void FreeSemaphore(struct SEM * Semaphore)
{
	//if (Semaphore->Flag == 0)
	//	Debugprintf("Free Semaphore Called when Sem not held");

	Semaphore->Rels++;
	Semaphore->Flag = 0;

	return;
}

void printStack(void)
{
}

pthread_t ResolveUpdateThreadId = 0;

char NodeMapServer[80] = "update.g8bpq.net";
char ChatMapServer[80] = "chatupdate.g8bpq.net";

VOID ResolveUpdateThread(void * Unused)
{
	struct hostent * HostEnt1;
	struct hostent * HostEnt2;

	ResolveUpdateThreadId = GetCurrentThreadId();

	while (TRUE)
	{

		Sleep(1000 * 60 * 5);
	}
}


VOID OpenReportingSockets()
{

}

VOID WriteMiniDumpThread();

time_t lastMiniDump = 0;

void WriteMiniDump()
{
}

// UI Util Code

#pragma pack(1)

typedef struct _MESSAGEX
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGEX * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID;
	UCHAR	DATA[256];
	UCHAR	PADDING[56];			// In case he have Digis

}MESSAGEX, *PMESSAGEX;

#pragma pack()


int PortNum[MaxBPQPortNo + 1] = {0};	// Tab nunber to port

char * UIUIDigi[MaxBPQPortNo + 1]= {0};
char * UIUIDigiAX[MaxBPQPortNo + 1] = {0};		// ax.25 version of digistring
int UIUIDigiLen[MaxBPQPortNo + 1] = {0};			// Length of AX string

char UIUIDEST[MaxBPQPortNo + 1][11] = {0};		// Dest for Beacons

char UIAXDEST[MaxBPQPortNo + 1][7] = {0};


UCHAR FN[MaxBPQPortNo + 1][256];			// Filename
int Interval[MaxBPQPortNo + 1];			// Beacon Interval (Mins)
int MinCounter[MaxBPQPortNo + 1];			// Interval Countdown

BOOL SendFromFile[MaxBPQPortNo + 1];
char Message[MaxBPQPortNo + 1][1000];		// Beacon Text

VOID SendUIBeacon(int Port);

BOOL RunUI = TRUE;

VOID UIThread(void * Unused)
{
	int Port, MaxPorts = GetNumberofPorts();

	Sleep(60000);

	while (RunUI)
	{
		int sleepInterval = 60000;

		for (Port = 1; Port <= MaxPorts; Port++)
		{
			if (MinCounter[Port])
			{
				MinCounter[Port]--;

				if (MinCounter[Port] == 0)
				{
					MinCounter[Port] = Interval[Port];
					SendUIBeacon(Port);

					// pause beteen beacons but adjust sleep interval to suit

					Sleep(10000);
					sleepInterval -= 10000;
				}
			}
		}

		while (sleepInterval <= 0)		// just in case we have a crazy config
			sleepInterval += 60000;

		Sleep(sleepInterval);
	}
}

int UIRemoveLF(char * Message, int len)
{
	// Remove lf chars

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		*ptr2 = *ptr1;
	
		if (*ptr1 == '\r')
			if (*(ptr1+1) == '\n')
			{
				ptr1++;
				len--;
			}
		ptr1++;
		ptr2++;
	}

	return (int)(ptr2 - Message);
}




VOID UISend_AX_Datagram(UCHAR * Msg, DWORD Len, UCHAR Port, UCHAR * HWADDR, BOOL Queue)
{
	MESSAGEX AXMSG;
	PMESSAGEX AXPTR = &AXMSG;
	int DataLen = Len;
	struct PORTCONTROL * PORT = GetPortTableEntryFromSlot(Port);

	// Block includes the Msg Header (7 or 11 bytes), Len Does not!

	memcpy(AXPTR->DEST, HWADDR, 7);

	// Get BCALL or PORTCALL if set

	if (PORT && PORT->PORTBCALL[0])
		memcpy(AXPTR->ORIGIN, PORT->PORTBCALL, 7);
	else if (PORT && PORT->PORTCALL[0])
		memcpy(AXPTR->ORIGIN, PORT->PORTCALL, 7);
	else
		memcpy(AXPTR->ORIGIN, MYCALL, 7);

	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	if (UIUIDigi[Port])
	{
		// This port has a digi string

		int DigiLen = UIUIDigiLen[Port];
		UCHAR * ptr;

		memcpy(&AXPTR->CTL, UIUIDigiAX[Port], DigiLen);
		
		ptr = (UCHAR *)AXPTR;
		ptr += DigiLen;
		AXPTR = (PMESSAGEX)ptr;

		Len += DigiLen;
	}

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->DATA, Msg, DataLen);

//	if (Queue)
//		QueueRaw(Port, &AXMSG, Len + 16);
//	else
		SendRaw(Port, (char *)&AXMSG.DEST, Len + 16);

	return;

}



VOID SendUIBeacon(int Port)
{
	char UIMessage[1024];
	int Len = (int)strlen(Message[Port]);
	int Index = 0;

	if (SendFromFile[Port])
	{
		FILE * hFile;

		hFile = fopen(FN[Port], "rb");
	
		if (hFile == 0)
			return;

		Len = (int)fread(UIMessage, 1, 1024, hFile); 
		
		fclose(hFile);

	}
	else
		strcpy(UIMessage, Message[Port]);

	Len =  UIRemoveLF(UIMessage, Len);

	while (Len > 256)
	{
		UISend_AX_Datagram(&UIMessage[Index], 256, Port, UIAXDEST[Port], TRUE);
		Index += 256;
		Len -= 256;
		Sleep(2000);
	}
	UISend_AX_Datagram(&UIMessage[Index], Len, Port, UIAXDEST[Port], TRUE);
}


VOID SetupUI(int Port)
{
	char DigiString[100], * DigiLeft;

	ConvToAX25(UIUIDEST[Port], &UIAXDEST[Port][0]);

	UIUIDigiLen[Port] = 0;

	if (UIUIDigi[Port])
	{
		UIUIDigiAX[Port] = zalloc(100);
		strcpy(DigiString, UIUIDigi[Port]);
		DigiLeft = strlop(DigiString,',');

		while(DigiString[0])
		{
			ConvToAX25(DigiString, &UIUIDigiAX[Port][UIUIDigiLen[Port]]);
			UIUIDigiLen[Port] += 7;

			if (DigiLeft)
			{
				memmove(DigiString, DigiLeft, (int)strlen(DigiLeft) + 1);
				DigiLeft = strlop(DigiString,',');
			}
			else
				DigiString[0] = 0;
		}
	}
}


extern struct DATAMESSAGE * REPLYBUFFER;
char * __cdecl Cmdprintf(TRANSPORTENTRY * Session, char * Bufferptr, const char * format, ...);

void GetPortCTEXT(TRANSPORTENTRY * Session, char * Bufferptr, char * CmdTail, void * CMD)
{
	char FN[250];
	FILE *hFile;
	struct stat STAT;
	struct PORTCONTROL * PORT = PORTTABLE;
	char PortList[256] = "";

	while (PORT)
	{
		if (PORT->CTEXT)
		{
			free(PORT->CTEXT);
			PORT->CTEXT = 0;
		}

		if (BPQDirectory[0] == 0)
			sprintf(FN, "Port%dCTEXT.txt", PORT->PORTNUMBER);
		else
			sprintf(FN, "%s/Port%dCTEXT.txt", BPQDirectory, PORT->PORTNUMBER);

		if (stat(FN, &STAT) == -1)
		{
			PORT = PORT->PORTPOINTER;
			continue;
		}

		hFile = fopen(FN, "rb");

		if (hFile)
		{
			char * ptr;
			
			PORT->CTEXT = zalloc(STAT.st_size + 1);
			fread(PORT->CTEXT , 1, STAT.st_size, hFile); 
			fclose(hFile);
			
			// convert CRLF or LF to CR
	
			while (ptr = strstr(PORT->CTEXT, "\r\n"))
				memmove(ptr, ptr + 1, strlen(ptr));

			// Now has LF

			while (ptr = strchr(PORT->CTEXT, '\n'))
				*ptr = '\r';


			sprintf(PortList, "%s,%d", PortList, PORT->PORTNUMBER);
		}

		PORT = PORT->PORTPOINTER;
	}

	if (Session)
	{	
		Bufferptr = Cmdprintf(Session, Bufferptr, "CTEXT Read for ports %s\r", &PortList[1]);
		SendCommandReply(Session, REPLYBUFFER, (int)(Bufferptr - (char *)REPLYBUFFER));
	}
	else
		Debugprintf("CTEXT Read for ports %s\r", &PortList[1]);
}

// Get the current frequency for a port. This can get a bit complicated, especially if looking for centre freq
// rather than dial freq (as this depends on mode).
//
// Used for various reporting functions - MH, Maps, BBS New User message,

// I think I'll try PORT "PortFreq" setting first then if that isn't available  via rigcontrol.
// 
// For now at least will report dial freq if using RIGCONTROL

DllExport uint64_t APIENTRY GetPortFrequency(int PortNo, char * FreqString)
{
	struct PORTCONTROL * PORT = GetPortTableEntryFromPortNum(PortNo);
	double freq = 0.0;
	uint64_t freqint = 0;

	char * ptr;
	int  n = 3;

	FreqString[0] = 0;
	
	if (PORT == 0)
		return 0;

	if (PORT->PortFreq)
	{
		freqint = PORT->PortFreq;
		freq = freqint / 1000000.0;
	}
	
	sprintf(FreqString, "%.6f", freq);

	// Return 3 digits after . (KHz) unless more are significant

	ptr = &FreqString[strlen(FreqString) - 1];

	while (n-- && *(ptr) == '0')
		*ptr-- = 0;

	return freqint;
}

SOCKET OpenHTTPSock(char * Host)
{
	SOCKET sock = 0;
	return sock;
}

static char HeaderTemplate[] = "POST %s HTTP/1.1\r\n"
	"Accept: application/json\r\n"
//	"Accept-Encoding: gzip,deflate,gzip, deflate\r\n"
	"Content-Type: application/json\r\n"
	"Host: %s:%d\r\n"
	"Content-Length: %d\r\n"
	"User-Agent: %s%s\r\n"
//	"Expect: 100-continue\r\n"
	"\r\n";




void BuildPortMH(char * MHJSON, struct PORTCONTROL * PORT)
{
	struct tm * TM;
	static char MHTIME[50];
	time_t szClock;
	MHSTRUC * MH = PORT->PORTMHEARD;
	int count = MHENTRIES;
	char Normcall[20];
	int len;
	char * ptr;
	char mhstr[400];
	int i;
	char c;

	if (MH == NULL)
		return;

	while (count--)
	{
		if (MH->MHCALL[0] == 0)
			break;

		len = ConvFromAX25(MH->MHCALL, Normcall);
		Normcall[len] = 0;

		ptr = &MH->MHCALL[6];   // End of Address bit

		if ((*ptr & 1) == 0)
		{
			// at least one digi - which we are not going to include
			MH++;
			continue;
		}

		// validate call to prevent corruption of json

		for (i=0; i < len; i++)
		{
			c = Normcall[i];
			
			if (!isalnum(c) && !(c == '#') && !(c == ' ') && !(c == '-'))
				goto skipit;
		}


		//format TIME

		szClock = MH->MHTIME;
		TM = gmtime(&szClock);
		sprintf(MHTIME, "%d-%d-%d %02d:%02d:%02d",
			TM->tm_year+1900, TM->tm_mon + 1, TM->tm_mday, TM->tm_hour, TM->tm_min, TM->tm_sec);

		sprintf(mhstr, "{\"callSign\": \"%s\", \"port\": \"%d\", \"packets\": %d, \"lastHeard\": \"%s\" },\r\n" ,
			Normcall, PORT->PORTNUMBER, MH->MHCOUNT, MHTIME);

		strcat( MHJSON, mhstr );
skipit:
		MH++;
	}
}






