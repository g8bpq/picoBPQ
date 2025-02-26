//
// Prototypes for BPQ32 Node Functions
//

#define DllImport

#define EXCLUDEBITS


#include "compatbits.h"

#include "asmstrucs.h"

int CheckExcludeList(unsigned char *Call);

extern int ConvFromAX25(unsigned char *incall, unsigned char *outcall);
extern int ConvToAX25(unsigned char *callsign, unsigned char *ax25call);
extern int ConvToAX25Ex(unsigned char *callsign, unsigned char *ax25call);
extern int WritetoConsoleLocal(char *buff);
extern void Consoleprintf(const char *format, ...);
extern void FreeConfig();

extern void *InitializeExtDriver(PEXTPORTDATA PORTVEC);

extern void PutLengthinBuffer(PDATAMESSAGE buff, USHORT datalen);  // Needed for arm5 portability
extern int GetLengthfromBuffer(PDATAMESSAGE buff);
extern int IntDecodeFrame(MESSAGE *msg, char *buffer, time_t Stamp, uint64_t Mask, int APRS, int MCTL);
extern int IntSetTraceOptionsEx(uint64_t mask, int mtxparam, int mcomparam, int monUIOnly);
extern int CountBits64(uint64_t in);

#define GetBuff() _GetBuff(__FILE__, __LINE__)
#define ReleaseBuffer(s) _ReleaseBuffer(s, __FILE__, __LINE__)
#define CheckGuardZone() _CheckGuardZone(__FILE__, __LINE__)

#define Q_REM(s) _Q_REM(s, __FILE__, __LINE__)
#define Q_REM_NP(s) _Q_REM_NP(s, __FILE__, __LINE__)

#define C_Q_ADD(s, b) _C_Q_ADD(s, b, __FILE__, __LINE__)

void _CheckGuardZone(char *File, int Line);

void *_Q_REM(void **Q, char *File, int Line);
void *_Q_REM_NP(void *Q, char *File, int Line);

int _C_Q_ADD(void *Q, void *BUFF, char *File, int Line);

unsigned int _ReleaseBuffer(void *BUFF, char *File, int Line);

void *_GetBuff(char *File, int Line);
int _C_Q_ADD(void *PQ, void *PBUFF, char *File, int Line);

int C_Q_COUNT(void *Q);

DllExport char *APIENTRY GetApplCall(int Appl);
DllExport char *APIENTRY GetApplAlias(int Appl);
DllExport int APIENTRY FindFreeStream();
DllExport int APIENTRY DeallocateStream(int stream);
DllExport int APIENTRY SessionState(int stream, int *state, int *change);
DllExport int APIENTRY SetAppl(int stream, int flags, int mask);
DllExport int APIENTRY GetMsg(int stream, char *msg, int *len, int *count);
DllExport int APIENTRY GetConnectionInfo(int stream, char *callsign,
                                         int *port, int *sesstype, int *paclen,
                                         int *maxframe, int *l4window);


struct config_setting_t;

extern int GetIntValue(struct config_setting_t *group, char *name);
int GetStringValue(struct config_setting_t *group, char *name, char *value);
void SaveIntValue(struct config_setting_t *group, char *name, int value);
void SaveStringValue(struct config_setting_t *group, char *name, char *value);

int EncryptPass(char *Pass, char *Encrypt);
void DecryptPass(char *Encrypt, unsigned char *Pass, unsigned int len);
Dll void APIENTRY CreateOneTimePassword(char *Password, char *KeyPhrase, int TimeOffset);
Dll int APIENTRY CheckOneTimePassword(char *Password, char *KeyPhrase);

DllExport int APIENTRY TXCount(int stream);
DllExport int APIENTRY RXCount(int stream);
DllExport int APIENTRY MONCount(int stream);

void ReadNodes();
int BPQTRACE(MESSAGE *Msg, int APRS);

void Commandintr(TRANSPORTENTRY *Session, struct DATAMESSAGE *Buffer);

void PostStateChange(TRANSPORTENTRY *Session);

void InnerCommandintr(TRANSPORTENTRY *Session, struct DATAMESSAGE *Buffer);
void DoTheCommand(TRANSPORTENTRY *Session);
char *MOVEANDCHECK(TRANSPORTENTRY *Session, char *Bufferptr, char *Source, int Len);
void DISPLAYCIRCUIT(TRANSPORTENTRY *L4, char *Buffer);
char *FormatUptime(int Uptime);
char *strlop(char *buf, char delim);
int CompareCalls(unsigned char *c1, unsigned char *c2);

void PostDataAvailable(TRANSPORTENTRY *Session);
int WritetoConsoleLocal(char *buff);
char *CHECKBUFFER(TRANSPORTENTRY *Session, char *Bufferptr);
void CLOSECURRENTSESSION(TRANSPORTENTRY *Session);

void SendCommandReply(TRANSPORTENTRY *Session, struct DATAMESSAGE *Buffer, int Len);

struct PORTCONTROL *APIENTRY GetPortTableEntryFromPortNum(int portnum);

int cCOUNT_AT_L2(struct _LINKTABLE *LINK);
void SENDL4CONNECT(TRANSPORTENTRY *Session);

void CloseSessionPartner(TRANSPORTENTRY *Session);
int COUNTNODES();
int DecodeNodeName(char *NodeName, char *ptr);
;
void DISPLAYCIRCUIT(TRANSPORTENTRY *L4, char *Buffer);
int cCOUNT_AT_L2(struct _LINKTABLE *LINK);
void *zalloc(int len);
int FindDestination(unsigned char *Call, struct DEST_LIST **REQDEST);

int ProcessConfig();

void PUT_ON_PORT_Q(struct PORTCONTROL *PORT, MESSAGE *Buffer);
void CLEAROUTLINK(struct _LINKTABLE *LINK);
void TellINP3LinkGone(struct ROUTE *Route);
void CLEARACTIVEROUTE(struct ROUTE *ROUTE, int Reason);

// Reason Equates

#define NORMALCLOSE 0
#define RETRIEDOUT 1
#define SETUPFAILED 2
#define LINKLOST 3
#define LINKSTUCK 4

int COUNT_AT_L2(struct _LINKTABLE *LINK);
void SENDIDMSG();
void SENDBTMSG();
void INP3TIMER();
void REMOVENODE(dest_list *DEST);
int ACTIVATE_DEST(struct DEST_LIST *DEST);
void TellINP3LinkSetupFailed(struct ROUTE *Route);
int FindNeighbour(unsigned char *Call, int Port, struct ROUTE **REQROUTE);
void PROCROUTES(struct DEST_LIST *DEST, struct ROUTE *ROUTE, int Qual);
int L2SETUPCROSSLINK(PROUTE ROUTE);
void REMOVENODE(dest_list *DEST);
char *SetupNodeHeader(struct DATAMESSAGE *Buffer);
void L4CONNECTFAILED(TRANSPORTENTRY *L4);
int CountFramesQueuedOnSession(TRANSPORTENTRY *Session);
void CLEARSESSIONENTRY(TRANSPORTENTRY *Session);
void Debugprintf(const char *format, ...);

int APIENTRY Restart();
int APIENTRY Reboot();
int APIENTRY Reconfig();
Dll int APIENTRY SaveNodes();


struct SEM;

void GetSemaphore(struct SEM *Semaphore, int ID);
void FreeSemaphore(struct SEM *Semaphore);

Dll int APIENTRY SessionControl(int stream, int command, int Mask);

int OpenCOMPort(void *pPort, int speed, int SetDTR, int SetRTS, int Quiet, int Stopbits);
int ReadCOMBlock(int fd, char *Block, int MaxLength);
int WriteCOMBlock(int fd, char *Block, int BytesToWrite);
void CloseCOMPort(int fd);

void initUTF8();
int Is8Bit(unsigned char *cpt, int len);
int WebIsUTF8(unsigned char *ptr, int len);
int IsUTF8(unsigned char *ptr, int len);
int Convert437toUTF8(unsigned char *MsgPtr, int len, unsigned char *UTF);
int Convert1251toUTF8(unsigned char *MsgPtr, int len, unsigned char *UTF);
int Convert1252toUTF8(unsigned char *MsgPtr, int len, unsigned char *UTF);
int TrytoGuessCode(unsigned char *Char, int Len);


#define CMD_TO_APPL 1    // PASS COMMAND TO APPLICATION
#define MSG_TO_USER 2    // SEND 'CONNECTED' TO USER
#define MSG_TO_APPL 4    //	SEND 'CONECTED' TO APPL
#define CHECK_FOR_ESC 8  // Look for ^d (^D) to disconnect session)

#define UI 3
#define SABM 0x2F
#define DISC 0x43
#define DM 0x0F
#define UA 0x63
#define FRMR 0x87
#define RR 1
#define RNR 5
#define REJ 9

// V2.2 Types

#define SREJ 0x0D
#define SABME 0x6F
#define XID 0xAF
#define TEST 0xE3

#define SUPPORT2point2 1

// XID Optional Functions

#define OPMustHave 0x02A080  // Sync TEST 16 bit FCS Extended Address
#define OPSREJ 4
#define OPSREJMult 0x200000
#define OPREJ 2
#define OPMod8 0x400
#define OPMod128 0x800

#define BPQHOSTSTREAMS 64

extern TRANSPORTENTRY *L4TABLE;
extern unsigned char NEXTID;
extern int MAXCIRCUITS;
extern int L4DEFAULTWINDOW;
extern int L4T1;
extern APPLCALLS APPLCALLTABLE[];
extern char *APPLS;
extern int NEEDMH;
extern int RFOnly;

extern char SESSIONHDDR[];

extern unsigned char NEXTID;

extern struct ROUTE *NEIGHBOURS;
extern int MAXNEIGHBOURS;

extern struct ROUTE *NEIGHBOURS;
extern int ROUTE_LEN;
extern int MAXNEIGHBOURS;

extern struct DEST_LIST *DESTS;  // NODE LIST
extern struct DEST_LIST *ENDDESTLIST;
extern int DEST_LIST_LEN;
extern int MAXDESTS;  // MAX NODES IN SYSTEM

extern struct _LINKTABLE *LINKS;
extern int LINK_TABLE_LEN;
extern int MAXLINKS;



extern char MYCALL[];       //		DB	7 DUP (0)	; NODE CALLSIGN (BIT SHIFTED)
extern char MYALIASTEXT[];  //	{"      "	; NODE ALIAS (KEEP TOGETHER)

extern unsigned char MYCALLWITHALIAS[13];
extern APPLCALLS APPLCALLTABLE[NumberofAppls];

extern unsigned char MYNODECALL[];     // NODE CALLSIGN (ASCII)
extern char NODECALLLOPPED[];  // NODE CALLSIGN (ASCII). Null terminated
extern unsigned char MYNETROMCALL[];   // NETROM CALLSIGN (ASCII)

extern unsigned char NETROMCALL[];  // NETORM CALL (AX25)

extern void *FREE_Q;

extern struct PORTCONTROL *PORTTABLE;
extern int NUMBEROFPORTS;


extern int OBSINIT;     // INITIAL OBSOLESCENCE VALUE
extern int OBSMIN;      // MINIMUM TO BROADCAST
extern int L3INTERVAL;  // "NODES" INTERVAL IN MINS
extern int IDINTERVAL;  // "ID" BROADCAST INTERVAL
extern int BTINTERVAL;  // "BT" BROADCAST INTERVAL
extern int MINQUAL;     // MIN QUALITY FOR AUTOUPDATES
extern int HIDENODES;   // N * COMMAND SWITCH
extern int BBSQUAL;     // QUALITY OF BBS RELATIVE TO NODE

extern int NUMBEROFBUFFERS;  // PACKET BUFFERS
extern int PACLEN;           //MAX PACKET SIZE

//	L2 SYSTEM TIMER RUNS AT 3 HZ

extern int T3;  // LINK VALIDATION TIMER (3 MINS) (+ a bit to reduce RR collisions)

extern int L2KILLTIME;  // IDLE LINK TIMER (16 MINS)
extern int L3LIVES;     // MAX L3 HOPS
extern int L4N2;        // LEVEL 4 RETRY COUNT
extern int L4LIMIT;     // IDLE SESSION LIMIT - 15 MINS
extern int L4DELAY;     // L4 DELAYED ACK TIMER

extern int BBS;   // INCLUDE BBS SUPPORT
extern int NODE;  // INCLUDE SWITCH SUPPORT

extern int FULL_CTEXT;  // CTEXT ON ALL CONNECTS IF NZ


// Although externally streams are numbered 1 to 64, internally offsets are 0 - 63

extern BPQVECSTRUC DUMMYVEC;  // Needed to force correct order of following

extern BPQVECSTRUC BPQHOSTVECTOR[BPQHOSTSTREAMS + 5];

extern int NODEORDER;
extern unsigned char LINKEDFLAG;

extern unsigned char UNPROTOCALL[80];


extern char *INFOMSG;

extern char *CTEXTMSG;
extern int CTEXTLEN;

extern unsigned char MYALIAS[7];  // ALIAS IN AX25 FORM
extern unsigned char BBSALIAS[7];

extern void *TRACE_Q;  // TRANSMITTED FRAMES TO BE TRACED

extern char HEADERCHAR;  // CHAR FOR _NODE HEADER MSGS

extern int AUTOSAVE;  // AUTO SAVE NODES ON EXIT FLAG
extern int L4APPL;    // Application for BBSCALL/ALIAS connects
extern int CFLAG;     // C =HOST Command

extern void *IDMSG_Q;  // ID/BEACONS WAITING TO BE SENT

extern struct DATAMESSAGE BTHDDR;
extern struct _MESSAGE IDHDDR;

extern void *IDMSG;

extern int L3TIMER;  // TIMER FOR 'NODES' MESSAGE
extern int IDTIMER;  // TIMER FOR ID MESSAGE
extern int BTTIMER;  // TIMER FOR BT MESSAGE

extern int STATSTIME;


extern int IPRequired;
extern int MaxHops;
extern int MAXRTT;
extern USHORT CWTABLE[];
extern TRANSPORTENTRY *L4TABLE;
extern unsigned char ROUTEQUAL;
extern unsigned int BPQMsg;
extern unsigned char ExcludeList[];


extern APPLCALLS APPLCALLTABLE[];

extern char VersionStringWithBuild[];
extern char VersionString[];

extern int MAXHEARDENTRIES;
extern int MHLEN;

extern int APPL1;
extern int PASSCMD;
extern int NUMBEROFCOMMANDS;

extern char *ConfigBuffer;

extern char *WL2KReportLine[];

extern int QCOUNT, MAXBUFFS, MAXCIRCUITS, L4DEFAULTWINDOW, L4T1, CMDXLEN;
extern char CMDALIAS[ALIASLEN][NumberofAppls];

extern int SEMGETS;
extern int SEMRELEASES;
extern int SEMCLASHES;
extern int MINBUFFCOUNT;

extern unsigned char BPQDirectory[];
extern unsigned char BPQProgramDirectory[];

extern unsigned char WINMOR[];
extern unsigned char PACTORCALL[];

extern unsigned char MCOM;
extern unsigned char MUIONLY;
extern unsigned char MTX;
extern uint64_t MMASK;

extern unsigned char NODECALL[];  //  NODES in ax.25

extern int L4CONNECTSOUT;
extern int L4CONNECTSIN;
extern int L4FRAMESTX;
extern int L4FRAMESRX;
extern int L4FRAMESRETRIED;
extern int OLDFRAMES;
extern int L3FRAMES;

extern char *PortConfig[];
extern struct SEM Semaphore;
extern unsigned char AuthorisedProgram;  // Local Variable. Set if Program is on secure list

extern int REALTIMETICKS;

extern time_t CurrentSecs;
extern time_t lastSlowSecs;
extern time_t last15Mins;

// SNMP Variables

extern int InOctets[64];
extern int OutOctets[64];

extern int CloseAllNeeded;
extern int CloseOnError;

extern char *PortConfig[70];
extern struct TNCINFO *TNCInfo[71];  // Records are Malloc'd

#define MaxBPQPortNo 10  // Port 64 reserved for BBS Mon
#define MAXBPQPORTS 10

// IP, APRS use port ocnfig slots above the real port range

#define IPConfigSlot MaxBPQPortNo + 1
#define PortMapConfigSlot MaxBPQPortNo + 2
#define APRSConfigSlot MaxBPQPortNo + 3


extern char *UIUIDigi[MaxBPQPortNo + 1];
extern char UIUIDEST[MaxBPQPortNo + 1][11];   // Dest for Beacons
extern unsigned char FN[MaxBPQPortNo + 1][256];       // Filename
extern int Interval[MaxBPQPortNo + 1];        // Beacon Interval (Mins)
extern char Message[MaxBPQPortNo + 1][1000];  // Beacon Text

extern int MinCounter[MaxBPQPortNo + 1];  // Interval Countdown
extern int SendFromFile[MaxBPQPortNo + 1];

extern int MQTT;
extern char MQTT_HOST[80];
extern int MQTT_PORT;
extern char MQTT_USER[80];
extern char MQTT_PASS[80];

uint64_t GetPortFrequency(int PortNo, char *FreqStringMhz);

void hookL2SessionAccepted(int Port, char * remotecall, char * ourcall, struct _LINKTABLE * LINK);
void hookL2SessionAttempt(int Port, char * ourcall, char * remotecall, struct _LINKTABLE * LINK);
void hookL2SessionDeleted(struct _LINKTABLE * LINK);
