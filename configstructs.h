
#ifndef CONFIGSTRUCT
#define CONFIGSTRUCT


// MAKE SURE SHORTS ARE CORRECTLY ALLIGNED FOR ARMV5


struct CFGX
{
	char String[20];			// COMMAND STRING
	UCHAR CMDLEN;				// SIGNIFICANT LENGTH
	int(*CMDPROC)();			// COMMAND PROCESSOR
	size_t CMDFLAG;				// FLAG/VALUE Offset

};


struct PORTCONFIG
{
	short PORTNUM;
	char ID[30];			//2
	short TYPE;			    // 32,
	short PROTOCOL;			// 34,
	short IOADDR;			// 36,
	short INTLEVEL;			// 38,
	unsigned short SPEED;	// 40,
	unsigned char CHANNEL;	// 42,
	unsigned char pad;
	short BBSFLAG;			// 44, 
	short QUALITY;			// 46, 
	short MAXFRAME;			// 48,
	short TXDELAY;			// 50,
	short SLOTTIME;			// 52, 
	short PERSIST;			// 54,

	short FULLDUP;			// 56,
	short SOFTDCD;			// 58, 
	short FRACK;			// 60, 
	short RESPTIME;			// 62,
	short RETRIES;			// 64, 

	short PACLEN;			// 66,
	short QUALADJUST;		// 68,
	UCHAR DIGIFLAG;			// 70,
	UCHAR DIGIPORT;			// 71 
	short DIGIMASK;			// 72
	short USERS;			// 74,
	short TXTAIL;			// 76
	unsigned char  ALIAS_IS_BBS;		// 78
	unsigned char pad2;
	char CWID[10];			// 80,
	char PORTCALL[10];		//  90,
	char PORTALIAS[10];		// 100,
	char L3ONLY;			// 110,
	char IGNOREUNLOCKED;	// 111
	short KISSOPTIONS;		// 112,
	short INTERLOCK;		// 114,
	short NODESPACLEN;		// 116,
	short TXPORT;			// 118,
	UCHAR MHEARD;			// 120,
	UCHAR CWIDTYPE;			// 121,
	char MINQUAL;			// 122, 
	char MAXDIGIS;			//  123,
	char DefaultNoKeepAlives; // 124
	char UIONLY;			// 125,
	unsigned short ListenPort;	// 126
	char UNPROTO[72];		//  128, 
	char PORTALIAS2[10];	//  200,
	char DLLNAME[16];		//  210,
	char BCALL[10];			// 226,
	unsigned long IPADDR;	// 236
	char I2CMode;			// 240
	char I2CAddr;			// 241
	char INP3ONLY;			// 242
	char NoNormalize;		// 243 Normalise Nodes Qualities
	unsigned short TCPPORT;	// 244
	char Pad2[10];			// 246
	char VALIDCALLS[256];	//   256 - 512
	struct WL2KInfo * WL2K;			// 512
	char SerialPortName[80]; // 516
	struct XDIGI * XDIGIS;	//  596 Cross port digi setup
	int RIGPORT;			// Linked port with RigControl 
	unsigned int PERMITTEDAPPLS;	// Appls allowed on this port
	int HavePermittedAppls;			// Indicated PERMITTEDAPPLS uses
	int Hide;				// Don't show on Ports display or AGW Connect Menu
//	long long txOffset;		// Transverter tx offset 
//	long long rxOffset;		// Transverter rx offset ppa
	int SmartID;
	unsigned char * KissParams;
	int SendtoM0LTEMap;
	uint64_t PortFreq;
	char * M0LTEMapInfo;
	int QtSMPort;
};

struct ROUTECONFIG
{
	char call[80];		// May have VIA
	int quality;
	int port;
	int pwind;
	int pfrack;
	int ppacl;
	int farQual;
};

struct CONFIGTABLE
{
//	CONFIGURATION DATA STRUCTURE

//	DEFINES LAYOUT OF CONFIG RECORD PRODUCED BY CONFIGURATION PROG

	char C_NODECALL[10];		// OFFSET = 0 
	char C_NODEALIAS[10];		// OFFSET = 10
	short C_OBSINIT;			// OFFSET = 40
	short C_OBSMIN;				// OFFSET = 42
	short C_NODESINTERVAL;		// OFFSET = 44
	short C_L3TIMETOLIVE;		// OFFSET = 46
	short C_L4RETRIES;			// OFFSET = 48
	short C_L4TIMEOUT;			// OFFSET = 50
	short C_BUFFERS;			// OFFSET = 52
	short C_PACLEN;				// OFFSET = 54
	short C_TRANSDELAY;			// OFFSET = 56
	short C_T3;					// OFFSET = 58
	short C_IDLETIME;			// OFFSET = 64
	short C_LINKEDFLAG;			// OFFSET = 67
	short C_MAXLINKS;			// OFFSET = 72
	short C_MAXDESTS;
	short C_MAXNEIGHBOURS;
	short C_MAXCIRCUITS;		// 78
	short C_IDINTERVAL;			// 96
	short C_FULLCTEXT;			// 98    ; SPARE (WAS DIGIFLAG)
	short C_MINQUAL;			// 100
	short C_HIDENODES;			// 102
	short C_AUTOSAVE;			// 103
	short C_L4DELAY;			// 104
	short C_L4WINDOW;			// 106
	short C_BTINTERVAL;			// 108
	short C_IP;					//  112 IP Enabled
	short C_MAXRTT;				// 113
	short C_MAXHOPS;			// 114
	short C_LogL4Connects;		// 116
	short C_SaveMH;				// 117
	short C_WASUNPROTO;
	char C_BTEXT[120];			// 121
	char C_VERSTRING[10];		// 241 Version String from Config File
	short C_EVENTS;
	short C_LogAllConnects;	
	short C_SaveAPRSMsgs;
	short C_M0LTEMap;
	short C_VERSION;			// CONFIG PROG VERSION
//	Reuse C_APPLICATIONS - no longer used
	char C_NETROMCALL[10];
	char C_EXCLUDE[80];
	char * C_IDMSG;
	char * C_CTEXT;
	char * C_INFOMSG;
	short CfgBridgeMap[MaxBPQPortNo + 1][MaxBPQPortNo + 1];
	struct ROUTECONFIG C_ROUTE[MaxLockedRoutes];
	struct PORTCONFIG C_PORT[MaxBPQPortNo + 4];
	short C_MQTT;
	char C_MQTT_HOST[80];
	short C_MQTT_PORT;
	char C_MQTT_USER[80];
	char C_MQTT_PASS[80];
	char * C_PASSWORD;
	char C_LOCATOR[10];
	char * C_MAPCOMMENT;

};
struct UPNP 
{
	struct UPNP * Next;
	char * Protocol;
	char * LANport;
	char * WANPort;
};

#endif



