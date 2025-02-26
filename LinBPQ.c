/*
Copyright 2001-2018 John Wiseman G8BPQ

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

// Control Routine for LinBPQ

#define _CRT_SECURE_NO_DEPRECATE

#include "CHeaders.h"

#include <iconv.h>
#include <errno.h>
#include "time.h"

#define ReturntoNode(stream) SessionControl(stream, 3, 0)
#define ConnectUsingAppl(stream, appl) SessionControl(stream, 0, appl)


void GetSemaphore(struct SEM *Semaphore, int ID);
void FreeSemaphore(struct SEM *Semaphore);
VOID CopyConfigFile(char *ConfigName);
VOID SendMailForThread(VOID *Param);
VOID GetUIConfig();
Dll BOOL APIENTRY Init_IP();
VOID OpenReportingSockets();
VOID SetupNTSAliases(char *FN);
int DeleteRedundantMessages();
VOID FindLostBuffers();
VOID IPClose();
DllExport BOOL APIENTRY Rig_Close();
Dll BOOL APIENTRY Poll_IP();
BOOL Rig_Poll();
BOOL Rig_Poll();
VOID CheckWL2KReportTimer();
VOID TNCTimer();
VOID SendLocation();
int ChatPollStreams();
void ChatTrytoSend();
VOID BBSSlowTimer();
int GetHTMLForms();
char *AddUser(char *Call, char *password, BOOL BBSFlag);

VOID SaveMH();



extern uint64_t timeLoadedMS;

BOOL IncludesMail = FALSE;
BOOL IncludesChat = FALSE;

BOOL RunMail = FALSE;
BOOL RunChat = FALSE;
BOOL needAIS = FALSE;
BOOL needADSB = FALSE;

int CloseOnError = 0;

VOID Poll_AGW();
BOOL AGWAPIInit();
int AGWAPITerminate();

BOOL AGWActive = FALSE;


BOOL RigActive = FALSE;

extern ULONG ChatApplMask;
extern char Verstring[];

extern char SignoffMsg[];
extern char AbortedMsg[];
extern char InfoBoxText[];  // Text to display in Config Info Popup

extern int LastVer[4];  // In case we need to do somthing the first time a version is run

extern HWND MainWnd;
extern char BaseDir[];
extern char BaseDirRaw[];
extern char MailDir[];
extern char WPDatabasePath[];
extern char RlineVer[50];

extern BOOL LogBBS;
extern BOOL LogCHAT;
extern BOOL LogTCP;
extern BOOL ForwardToMe;

extern int LatestMsg;
extern char BBSName[];
extern char SYSOPCall[];
extern char BBSSID[];
extern char NewUserPrompt[];

extern int NumberofStreams;
extern int MaxStreams;
extern ULONG BBSApplMask;
extern int BBSApplNum;
extern int ChatApplNum;
extern int MaxChatStreams;

extern int NUMBEROFTNCPORTS;

extern int EnableUI;

extern BOOL AUTOSAVEMH;


time_t LastTrafficTime;
extern int MaintTime;

#define LOG_BBS 0
#define LOG_CHAT 1
#define LOG_TCP 2
#define LOG_DEBUG_X 3

int _MYTIMEZONE = 0;

// flags equates

#define F_Excluded 0x0001
#define F_LOC 0x0002
#define F_Expert 0x0004
#define F_SYSOP 0x0008
#define F_BBS 0x0010
#define F_PAG 0x0020
#define F_GST 0x0040
#define F_MOD 0x0080
#define F_PRV 0x0100
#define F_UNP 0x0200
#define F_NEW 0x0400
#define F_PMS 0x0800
#define F_EMAIL 0x1000
#define F_HOLDMAIL 0x2000
#define F_POLLRMS 0x4000
#define F_SYSOP_IN_LM 0x8000
#define F_Temp_B2_BBS 0x00010000

/* #define F_PWD        0x1000 */

BOOL GetConfig(char *ConfigName);
VOID DecryptPass(char *Encrypt, unsigned char *Pass, unsigned int len);
int EncryptPass(char *Pass, char *Encrypt);
int APIENTRY FindFreeStream();
int PollStreams();
int APIENTRY SetAppl(int stream, int flags, int mask);
int APIENTRY SessionState(int stream, int *state, int *change);
int APIENTRY SessionControl(int stream, int command, int Mask);

BOOL ChatInit();
VOID CloseChat();
VOID CloseTNCEmulator();

extern BOOL LogAPRSIS;

BOOL UIEnabled[33];
BOOL UINull[33];
char *UIDigi[33];

extern struct UserInfo **UserRecPtr;
extern int NumberofUsers;

extern struct UserInfo *BBSChain;  // Chain of users that are BBSes

extern struct MsgInfo **MsgHddrPtr;
extern int NumberofMessages;

extern time_t MaintClock;  // Time to run housekeeping

#ifdef WIN32
BOOL KEEPGOING = 30;  // 5 secs to shut down
#else
BOOL KEEPGOING = 50;  // 5 secs to shut down
#endif
BOOL Restarting = FALSE;
BOOL CLOSING = FALSE;

int ProgramErrors;
int Slowtimer = 0;

#define REPORTINTERVAL 15 * 549;  // Magic Ticks Per Minute for PC's nominal 100 ms timer
int ReportTimer = 0;

#define MAXSTACK 20
#define INPUTLEN 512

// Console Terminal Support

struct ConTermS 
{
  int BPQStream;
  BOOL Active;
  int Incoming;

  char kbbuf[INPUTLEN];
  int kbptr;

  char *KbdStack[MAXSTACK];
  int StackIndex;

  BOOL CONNECTED;
  int SlowTimer;
};

struct ConTermS ConTerms[3] = { 0, 0 };


extern void OutputDebugString(char *Mess);

VOID __cdecl Debugprintf(const char *format, ...);

VOID *zalloc(int len);

int WritetoConsoleLocal(char *buff);


char *stristr(char *ch1, char *ch2) {
  char *chN1, *chN2;
  char *chNdx;
  char *chRet = NULL;

  chN1 = _strdup(ch1);
  chN2 = _strdup(ch2);

  if (chN1 && chN2) {
    chNdx = chN1;
    while (*chNdx) {
      *chNdx = (char)tolower(*chNdx);
      chNdx++;
    }
    chNdx = chN2;

    while (*chNdx) {
      *chNdx = (char)tolower(*chNdx);
      chNdx++;
    }

    chNdx = strstr(chN1, chN2);

    if (chNdx)
      chRet = ch1 + (chNdx - chN1);
  }

  free(chN1);
  free(chN2);
  return chRet;
}


VOID SendSmartID(struct PORTCONTROL *PORT) {
  struct _MESSAGE *ID = IDMSG;
  struct _MESSAGE *Buffer;

  PORT->SmartIDNeeded = 0;

  Buffer = GetBuff();

  if (Buffer) {
    memcpy(Buffer, ID, ID->LENGTH);

    Buffer->PORT = PORT->PORTNUMBER;

    //	IF PORT HAS A CALLSIGN DEFINED, SEND THAT INSTEAD

    if (PORT->PORTCALL[0] > 0x40) {
      memcpy(Buffer->ORIGIN, PORT->PORTCALL, 7);
      Buffer->ORIGIN[6] |= 1;  // SET END OF CALL BIT
    }

    PUT_ON_PORT_Q(PORT, Buffer);
  }
}


#define FEND 0xC0  // KISS CONTROL CODES
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD


int KissEncode(UCHAR *inbuff, UCHAR *outbuff, int len) {
  int i, txptr = 0;
  UCHAR c;

  outbuff[0] = FEND;
  txptr = 1;

  for (i = 0; i < len; i++) {
    c = inbuff[i];

    switch (c) {
      case FEND:
        outbuff[txptr++] = FESC;
        outbuff[txptr++] = TFEND;
        break;

      case FESC:

        outbuff[txptr++] = FESC;
        outbuff[txptr++] = TFESC;
        break;

      default:

        outbuff[txptr++] = c;
    }
  }

  outbuff[txptr++] = FEND;

  return txptr;
}

VOID Send_AX(UCHAR *Block, DWORD Len, UCHAR Port) 
{
  // Block included the 7/11 byte header, Len does not

  struct PORTCONTROL *PORT;
  PMESSAGE Copy;

  if (Len > 360 - 15)
    return;

  if (QCOUNT < 20)
    return;  // Running low

  PORT = GetPortTableEntryFromPortNum(Port);

  if (PORT == 0)
    return;

  Copy = GetBuff();

  if (Copy == 0)
    return;

  memcpy(&Copy->DEST[0], &Block[MSGHDDRLEN], Len);
  Copy->LENGTH = (USHORT)Len + MSGHDDRLEN;
  Copy->PORT = Port;

  PUT_ON_PORT_Q(PORT, Copy);
}


VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port) 
{
  //	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

  //  Len is the Payload Length (CTL, PID, Data)

  // The message can contain DIGIS - The payload must be copied forwards if there are less than 8

  UCHAR *EndofDigis = &Block->CTL;

  int i = 0;

  while (Block->DIGIS[i][0] && i < 8) {
    i++;
  }

  EndofDigis = &Block->DIGIS[i][0];
  *(EndofDigis - 1) |= 1;  // Set End of Address Bit

  if (i != 8)
    memmove(EndofDigis, &Block->CTL, Len);  // Include PID

  Len = Len + (i * 7) + 14;  // Include Source, Dest and Digis

  Send_AX((unsigned char *)Block, Len, Port);
  return;
}


VOID SENDIDMSG() {
  struct PORTCONTROL *PORT = PORTTABLE;
  struct _MESSAGE *ID = IDMSG;
  struct _MESSAGE *Buffer;

  while (PORT) {
    if (PORT->PROTOCOL < 10)  // Not Pactor-style
    {
      Buffer = GetBuff();

      if (Buffer) {
        memcpy(Buffer, ID, ID->LENGTH);

        Buffer->PORT = PORT->PORTNUMBER;

        //	IF PORT HAS A CALLSIGN DEFINED, SEND THAT INSTEAD

        if (PORT->PORTCALL[0] > 0x40) {
          memcpy(Buffer->ORIGIN, PORT->PORTCALL, 7);
          Buffer->ORIGIN[6] |= 1;  // SET END OF CALL BIT
        }
        C_Q_ADD(&IDMSG_Q, Buffer);
      }
    }
    PORT = PORT->PORTPOINTER;
  }
}


VOID SENDBTMSG() {
  struct PORTCONTROL *PORT = PORTTABLE;
  struct _MESSAGE *Buffer;
  char *ptr1, *ptr2;

  while (PORT) {
    if (PORT->PROTOCOL >= 10 || PORT->PORTUNPROTO == 0)  // Pactor-style or no UNPROTO ADDR?
    {
      PORT = PORT->PORTPOINTER;
      continue;
    }

    Buffer = GetBuff();

    if (Buffer) {
      memcpy(Buffer->DEST, PORT->PORTUNPROTO, 7);
      Buffer->DEST[6] |= 0xC0;  // Set Command bits

      //	Send from BBSCALL unless PORTBCALL defined

      if (PORT->PORTBCALL[0] > 32)
        memcpy(Buffer->ORIGIN, PORT->PORTBCALL, 7);
      else if (APPLCALLTABLE->APPLCALL[0] > 32)
        memcpy(Buffer->ORIGIN, APPLCALLTABLE->APPLCALL, 7);
      else
        memcpy(Buffer->ORIGIN, MYCALL, 7);

      ptr1 = &PORT->PORTUNPROTO[7];  // First Digi
      ptr2 = &Buffer->CTL;           // Digi field in buffer

      // Copy any digis

      while (*(ptr1)) {
        memcpy(ptr2, ptr1, 7);
        ptr1 += 7;
        ptr2 += 7;
      }

      *(ptr2 - 1) |= 1;  // Set End of Address
      *(ptr2++) = UI;

      memcpy(ptr2, &BTHDDR.PID, BTHDDR.LENGTH);
      ptr2 += BTHDDR.LENGTH;
      Buffer->LENGTH = (int)(ptr2 - (char *)Buffer);
      Buffer->PORT = PORT->PORTNUMBER;

      C_Q_ADD(&IDMSG_Q, Buffer);
    }
    PORT = PORT->PORTPOINTER;
  }
}

VOID SENDUIMESSAGE(struct DATAMESSAGE *Msg) {
  struct PORTCONTROL *PORT = PORTTABLE;
  struct _MESSAGE *Buffer;
  char *ptr1, *ptr2;

  Msg->LENGTH -= MSGHDDRLEN;  // Remove Header

  while (PORT) {
    if ((PORT->PROTOCOL == 10 && PORT->UICAPABLE == 0) || PORT->PORTUNPROTO == 0)  // Pactor-style or no UNPROTO ADDR?
    {
      PORT = PORT->PORTPOINTER;
      continue;
    }

    Buffer = GetBuff();

    if (Buffer) {
      memcpy(Buffer->DEST, PORT->PORTUNPROTO, 7);
      Buffer->DEST[6] |= 0xC0;  // Set Command bits

      //	Send from BBSCALL unless PORTBCALL defined

      if (PORT->PORTBCALL[0] > 32)
        memcpy(Buffer->ORIGIN, PORT->PORTBCALL, 7);
      else if (APPLCALLTABLE->APPLCALL[0] > 32)
        memcpy(Buffer->ORIGIN, APPLCALLTABLE->APPLCALL, 7);
      else
        memcpy(Buffer->ORIGIN, MYCALL, 7);

      ptr1 = &PORT->PORTUNPROTO[7];  // First Digi
      ptr2 = &Buffer->CTL;           // Digi field in buffer

      // Copy any digis

      while (*(ptr1)) {
        memcpy(ptr2, ptr1, 7);
        ptr1 += 7;
        ptr2 += 7;
      }

      *(ptr2 - 1) |= 1;  // Set End of Address
      *(ptr2++) = UI;

      memcpy(ptr2, &Msg->PID, Msg->LENGTH);
      ptr2 += Msg->LENGTH;
      Buffer->LENGTH = (int)(ptr2 - (char *)Buffer);
      Buffer->PORT = PORT->PORTNUMBER;

      C_Q_ADD(&IDMSG_Q, Buffer);
    }
    PORT = PORT->PORTPOINTER;
  }
}

void hookL2SessionAccepted(int Port, char *remotecall, char *ourcall, struct _LINKTABLE *LINK) {}
void hookL2SessionAttempt(int Port, char *ourcall, char *remotecall, struct _LINKTABLE *LINK) {}
void hookL2SessionDeleted(struct _LINKTABLE *LINK) {}

int KissDecode(UCHAR *inbuff, int len) {
  int i, txptr = 0;
  UCHAR c;

  for (i = 0; i < len; i++) {
    c = inbuff[i];

    if (c == FESC) {
      c = inbuff[++i];
      {
        if (c == TFESC)
          c = FESC;
        else if (c == TFEND)
          c = FEND;
      }
    }
    inbuff[txptr++] = c;
  }
  return txptr;
}


TRANSPORTENTRY *SetupSessionFromHost(PBPQVECSTRUC HOST, UINT ApplMask) 
{
  // Create a Transport (L4) session linked to an incoming HOST (API) Session

  TRANSPORTENTRY *NewSess = L4TABLE;
  int Index = 0;

  while (Index < MAXCIRCUITS) {
    if (NewSess->L4USER[0] == 0) {
      // Got One

      UCHAR *ourcall = &MYCALL[0];

      // IF APPL PORT USE APPL CALL, ELSE NODE CALL

      if (ApplMask) {
        // Circuit for APPL - look for an APPLCALL

        APPLCALLS *APPL = APPLCALLTABLE;

        while ((ApplMask & 1) == 0) {
          ApplMask >>= 1;
          APPL++;
        }
        if (APPL->APPLCALL[0] > 0x40)  // We have an applcall
          ourcall = &APPL->APPLCALL[0];
      }

      memcpy(NewSess->L4USER, ourcall, 7);
      memcpy(NewSess->L4MYCALL, ourcall, 7);

      NewSess->CIRCUITINDEX = Index;  //OUR INDEX
      NewSess->CIRCUITID = NEXTID;

      NEXTID++;
      if (NEXTID == 0)
        NEXTID++;  // Keep Non-Zero

      NewSess->L4TARGET.HOST = HOST;
      NewSess->L4STATE = 5;


      NewSess->SESSIONT1 = L4T1;
      NewSess->L4WINDOW = (UCHAR)L4DEFAULTWINDOW;
      NewSess->SESSPACLEN = PACLEN;  // Default;

      return NewSess;
    }
    Index++;
    NewSess++;
  }

  // Table Full

  return NULL;
}



#ifndef WIN32

BOOL CopyFile(char *In, char *Out, BOOL Failifexists) {
  FILE *Handle;
  DWORD FileSize;
  char *Buffer;
  struct stat STAT;

  if (stat(In, &STAT) == -1)
    return FALSE;

  FileSize = STAT.st_size;

  Handle = fopen(In, "rb");

  if (Handle == NULL)
    return FALSE;

  Buffer = malloc(FileSize + 1);

  FileSize = fread(Buffer, 1, STAT.st_size, Handle);

  fclose(Handle);

  if (FileSize != STAT.st_size) {
    free(Buffer);
    return FALSE;
  }

  Handle = fopen(Out, "wb");

  if (Handle == NULL) {
    free(Buffer);
    return FALSE;
  }

  FileSize = fwrite(Buffer, 1, STAT.st_size, Handle);

  fclose(Handle);
  free(Buffer);

  return TRUE;
}
#endif

int RefreshMainWindow() {
  return 0;
}

int LastSemGets = 0;

extern int SemHeldByAPI;

VOID MonitorThread(void *x) {
  // Thread to detect stuck semaphore

  do {
    if ((Semaphore.Gets == LastSemGets) && Semaphore.Flag) {
      // It is stuck - try to release

      Debugprintf("Semaphore locked - Process ID = %d, Held By %d",
                  Semaphore.SemProcessID, SemHeldByAPI);

      Semaphore.Flag = 0;
    }

    LastSemGets = Semaphore.Gets;

    Sleep(30000);
    //		Debugprintf("Monitor Thread Still going %d %d %d %x %d", LastSemGets, Semaphore.Gets, Semaphore.Flag, Semaphore.SemThreadID, SemHeldByAPI);

  } while (TRUE);
}




VOID TIMERINTERRUPT();

BOOL Start();
VOID INITIALISEPORTS();
Dll BOOL APIENTRY Init_APRS();
VOID APRSClose();
Dll VOID APIENTRY Poll_APRS();
VOID HTTPTimer();


#define CKernel
#include "Versions.h"

extern struct SEM Semaphore;

int SemHeldByAPI = 0;
BOOL IGateEnabled = TRUE;
BOOL APRSActive = FALSE;
BOOL ReconfigFlag = FALSE;
BOOL APRSReconfigFlag = FALSE;
BOOL RigReconfigFlag = FALSE;

BOOL IPActive = FALSE;
extern BOOL IPRequired;

extern struct WL2KInfo *WL2KReports;

int InitDone;
char pgm[256] = "LINBPQ";

char SESSIONHDDR[80] = "";
int SESSHDDRLEN = 0;

// Next 3 should be uninitialised so they are local to each process

UCHAR MCOM;
UCHAR MUIONLY;
UCHAR MTX;
uint64_t MMASK;

UCHAR AuthorisedProgram = 1;  // Local Variable. Set if Program is on secure list

int SAVEPORT = 0;

char VersionString[50] = Verstring;
char VersionStringWithBuild[50] = Verstring;
int Ver[4] = { Vers };
char TextVerstring[50] = Verstring;

extern UCHAR PWLen;
extern char PWTEXT[];
extern int ISPort;

extern char ChatConfigName[250];

BOOL EventsEnabled = 0;

// Console Terminal Stuff

int SendMsg(int stream, char *msg, int len);

int ConTermKbhit(int n);
void Contermprint(int n, char * Msg);

void ConTermInput(int n, char * Msg)
{
  struct ConTermS *Conterm = &ConTerms[n];
  int i;

  if (Conterm->BPQStream == 0) 
  {
    Conterm->BPQStream = FindFreeStream();

    if (Conterm->BPQStream == 255) 
    {
      Conterm->BPQStream = 0;
      Contermprint(n, "No Free Streams\n");
      return;
    }
  }

  if (!Conterm->CONNECTED)
    SessionControl(Conterm->BPQStream, 1, 0);

  if (n == 2)       // Modem Stream
  {
    SendMsg(Conterm->BPQStream, Msg, strlen(Msg));
    return;
  }
  
  Conterm->StackIndex = 0;

  // Stack it

  if (Conterm->KbdStack[19])
    free(Conterm->KbdStack[19]);

  for (i = 18; i >= 0; i--) {
    Conterm->KbdStack[i + 1] = Conterm->KbdStack[i];
  }

  Conterm->KbdStack[0] = _strdup(Conterm->kbbuf);

  Conterm->kbbuf[Conterm->kbptr] = 13;

  SendMsg(Conterm->BPQStream, Conterm->kbbuf, Conterm->kbptr + 1);
}

void ConTermPoll() 
{
  int port, sesstype, paclen, maxframe, l4window, len;
  int state, change, InputLen, count;
  char callsign[11] = "";
  char Msg[300];
  int n;
  struct ConTermS *Conterm;

  // Third ConTerm can be hooked to the LTE modem on Serial6 to give dialup console access.

  for (n = 0; n < 3; n++)
  {
    Conterm = &ConTerms[n];

    //	Get current Session State. Any state changed is ACK'ed
    //	automatically. See BPQHOST functions 4 and 5.

    SessionState(Conterm->BPQStream, &state, &change);

    if (change == 1) {
      if (state == 1) {
        // Connected

        Conterm->CONNECTED = TRUE;
        Conterm->SlowTimer = 0;
      } else
       {
        Conterm->CONNECTED = FALSE;
        Contermprint(n, "*** Disconnected\n");
      }
    }

    GetMsg(Conterm->BPQStream, Msg, &InputLen, &count);

    if (InputLen)
     {
      char *ptr = Msg;
      char *ptr2 = ptr;
      Msg[InputLen] = 0;

      while (ptr) {
        ptr2 = strlop(ptr, 13);

        // Replace CR with CRLF

        Contermprint(n, ptr);
        if (ptr2)
          Contermprint(n, "\r\n");

        ptr = ptr2;
      }
    }

    if (n != 2)       // Don't poll 3rd here as is used for dialup modem
    {
      int c = ConTermKbhit(n);

      while (c != -1)
      {
        if (c == 8) 
        {
          if (Conterm->kbptr)
            Conterm->kbptr--;
          Contermprint(n, " \b");  // Already echoed bs - clear typed char from screen
          c = ConTermKbhit(n);
          continue;
        }

        if (c == 13 || c == 10)
        {
          Conterm->kbbuf[Conterm->kbptr] = 0;

          Contermprint(n, Conterm->kbbuf);
          Contermprint(n, "\r\n");
          ConTermInput(n, Conterm->kbbuf);
          Conterm->kbptr = 0;
          c = ConTermKbhit(n);
          continue;
        }
        Conterm->kbbuf[Conterm->kbptr++] = c;
        c = ConTermKbhit(n);
      }
    }
  }
}

int Redirected = 0;

int picoRename(char * from, char * to);
void picocreateDefaultcfg();

int BPQInit()
 {
  int i;
  struct UserInfo *user = NULL;
  struct stat STAT;
  PEXTPORTDATA PORTVEC;
 
  timeLoadedMS = GetTickCount();

  Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
  Consoleprintf("%s\n", VerCopyright);

  _MYTIMEZONE = 0;

  BPQDirectory[0] = 0;

  if (!ProcessConfig("bpq32.cfg"))
	{
		WritetoConsoleLocal("Main Configuration File Error - trying backup\n");
		if (!ProcessConfig("backup.cfg"))
		{
  		WritetoConsoleLocal("Backup Configuration File Error - creating default\n");

      picoRename("bpq32.cfg", "bpq32.cfg.bad");
      picocreateDefaultcfg();

      if (!ProcessConfig("bpq32.cfg"))
    	{
		    WritetoConsoleLocal("Default Configuration File Error - sending for help\n");
			  return 0;
      }
		}
	}

  SESSHDDRLEN = sprintf(SESSIONHDDR, "G8BPQ Network System %s for RP2040", TextVerstring);

  Debugprintf(SESSIONHDDR);

  GetSemaphore(&Semaphore, 0);

  if (Start() != 0) {
    FreeSemaphore(&Semaphore);
    return 0;
  }

  for (i = 0; PWTEXT[i] > 0x20; i++)
    ;  //Scan for cr or null

  PWLen = i;

  INITIALISEPORTS();

  FreeSemaphore(&Semaphore);

  initUTF8();
  InitDone = TRUE;

  ConTerms[0].BPQStream = FindFreeStream();
  ConTerms[1].BPQStream = FindFreeStream();
  ConTerms[2].BPQStream = FindFreeStream();

  return 1;
}

int WritetoConsoleLocal(char *buff);

int APIENTRY WritetoConsole(char *buff) {
  return WritetoConsoleLocal(buff);
}

void BPQFastPoll()
{
	int i;
	struct PORTCONTROL * PORT = PORTTABLE;

 	for (i = 0; i < NUMBEROFPORTS; i++)
	{	
		PORT->PORTRXROUTINE(PORT);			// SEE IF MESSAGE RECEIVED
		PORT = PORT->PORTPOINTER;
	}

  ConTermPoll();
}

int picoSerialGetLine(int Chan, char * Line, int maxline);

void BPQTimerLoop()
{
	TRANSPORTENTRY * L4 = L4TABLE;
	int n = MAXCIRCUITS;
	int rxed = 0;
  
	GetSemaphore(&Semaphore, 2);

	if (QCOUNT < 10) 
	{
		if (CLOSING == FALSE)
			FindLostBuffers();
		CLOSING = TRUE;
	}

	TIMERINTERRUPT();

	while (n--)
	{
		if (L4->DIAL == 0)
		{
			L4++;
			continue;
		}

    char Line[256];

    // Check LTE modem and send anything received to terminal
	
 //   int rxed = picoSerialGetLine(L4->DIAL, Line, 256);

    if (rxed)
    {
      struct DATAMESSAGE * Buffer = GetBuff();

      if (Buffer)
      {
	      Buffer->LENGTH = rxed + MSGHDDRLEN + 3;
        memcpy(Buffer->L2DATA, Line, rxed);

      	C_Q_ADD(&L4->L4TX_Q, (UINT *)Buffer);
      }
    }
		L4++;
	}

  FreeSemaphore(&Semaphore);

  Slowtimer++;

  if (Slowtimer > 100)
    Slowtimer = 0;
}

int sendtoDialSession(char * Line, int rxed)
{
	TRANSPORTENTRY * L4 = L4TABLE;
  int n = MAXCIRCUITS;
  struct DATAMESSAGE * Buffer;
  
	while (n--)
	{
		if (L4->DIAL)
		{
		  Buffer = GetBuff();

      if (Buffer)
      {
	      Buffer->LENGTH = rxed + MSGHDDRLEN + 3;
        memcpy(Buffer->L2DATA, Line, rxed);
      	C_Q_ADD(&L4->L4TX_Q, (UINT *)Buffer);
        return 1;
      }
    }
		L4++;
	}
  return 0;
}


int CountBits64(uint64_t in) {
  int n = 0;
  while (in) {
    if (in & 1) n++;
    in >>= 1;
  }
  return n;
}

int CountBits(uint32_t in) {
  int n = 0;
  while (in) {
    if (in & 1) n++;
    in >>= 1;
  }
  return n;
}


struct SEM Semaphore = { 0, 0, 0, 0 };

struct TNCINFO *TNC;

#ifndef WIN32

#include <time.h>
#include <sys/time.h>

#ifndef MACBPQ
#ifdef __MACH__

#include <mach/mach_time.h>

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0



int clock_gettime(int clk_id, struct timespec *t) {
  mach_timebase_info_data_t timebase;
  mach_timebase_info(&timebase);
  uint64_t time;
  time = mach_absolute_time();
  double nseconds = ((double)time * (double)timebase.numer) / ((double)timebase.denom);
  double seconds = ((double)time * (double)timebase.numer) / ((double)timebase.denom * 1e9);
  t->tv_sec = seconds;
  t->tv_nsec = nseconds;
  return 0;
}
#endif
#endif


void SetWindowText(HWND hWnd, char *lpString) {
  return;
};

BOOL SetDlgItemText(HWND hWnd, int item, char *lpString) {
  return 0;
};

#endif

int GetListeningPortsPID(int Port) {
  return 0;  // Not found
}



VOID Check_Timer() {
}

VOID POSTDATAAVAIL(){};


//VOID SendRPBeacon(struct TNCINFO * TNC)
//{
//}



VOID CloseConsole(int Stream) {
}

#ifndef WIN32

int V4ProcessReceivedData(struct TNCINFO *TNC) {
  return 0;
}
#endif

#ifdef FREEBSD

char *gcvt(double _Val, int _NumOfDigits, char *_DstBuf) {
  sprintf(_DstBuf, "%f", _Val);
  return _DstBuf;
}

#endif
