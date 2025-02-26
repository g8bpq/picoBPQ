#include "microOneWire.h"
#include "LittleFS.h"  // LittleFS is declared
#ifdef USE_TINYUSB
#include "Adafruit_TinyUSB.h"
#endif
#include <Time.h>

#include "RTClib.h"

// we support a number of rtc chips. functions are basically the same, so can just cast the contructor.
// exception is Millis (internal timer based) which has different begin params

RTC_Millis *rtcm = nullptr;
RTC_DS1307 *rtc = nullptr;
RTC_PCF8523 *rtcpcf;

extern "C" void Debugprintf(const char *format, ...);

// if using Adafruit USB stack can have multiple USB Serial ports (seems up to 3, including one uses as Serial)

#ifdef USE_TINYUSB
Adafruit_USBD_CDC USBSer1;
Adafruit_USBD_CDC USBSer2;
#endif

HardwareSerial *CONSOLE;
HardwareSerial *CONTERM[4] = { 0 };  // Give access to LinBPQ command handler
HardwareSerial *DEBUG;
HardwareSerial *LTESerial;

SerialPIO *Serial3 = nullptr;
SerialPIO *Serial4 = nullptr;
SerialPIO *Serial5 = nullptr;
SerialPIO *Serial6 = nullptr;

// Serial Port Support
// Pin Numbers

// These are GPIO pin numbers not board pin numbers
// Serial1 (0,1) SerialPIO (2,3 6,7) Serial2 (8,9) SerialPIO (10,11 12,13)

int SerTX[6] = { 0, 8, 2, 6, 10, 12 };

// DS1904 Defaults

int OWPIN = 22;  // Pin for DS1904 RTC

// RTC Equates

#define RTCNone 0
#define RTCLTE 1
#define RTCDS1904 2
#define RTCDS1307 4
#define RTDS3231 8
#define RTCPCF8523 16

int RTC = RTCDS1307;  //RTCDS1904;

// Clipper LTE Defaults

int PWRPIN = 14;    // Clipper LTE Power Pin
int RESETPIN = 15;  // Clipper LTE Reset Pin

int useClipper = 0;

//char apn[] = "iot.1nce.net"; PAP no user/pass

//char APN[64] = "data.uk";
//char LTEUser[64] = "user";
//char LTEPassword[64] = "one2one";

char SMSDEST[16] = "";
char ConnectOut1[64] = "" ; //"skig.g8bpq.net:8015";
char ConnectOut2[64] = ""; //"nottm.g8bpq.net:8015";

int SMSEvents = 0;

// Equates for SMSEvent bits

#define SMSonRestart 1
#define SMSonCallFailed 2

// Defaults - may be overriden by board config

char DefaultNodeCall[10] = "";  // Used if can't read config file

char Consolenames[3][32] = { "Serial:115200" };
char DebugPort[32] = "Serial:115200";
char ConsolePort[32] = "Serial:115200";

extern "C" void BPQTimerLoop();
extern "C" int BPQInit();
extern "C" void BPQFastPoll();
extern "C" char *strlop(char *buf, char delim);
extern "C" void ConTermInput(int n, char *Msg);
extern "C" char *stristr(char *ch1, char *ch2);
extern "C" int SaveMH();
extern "C" int SaveNodes();
extern "C" int stricmp(char *pStr1, const char *pStr2);

extern "C" char NODECALLLOPPED[10];

extern "C" const char *month[];
extern "C" const char *dat[];

extern "C" char MQTT_HOST[80];
extern "C" int MQTT_PORT;
extern "C" char MQTT_USER[80];
extern "C" char MQTT_PASS[80];

File cfg;

const char dummyConfig[] =
  "SIMPLE\r\n"
  "NODECALL=DUMMY\r\n"
  "NODEALIAS=RP2040\r\n"
  "PASSWORD=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n"
  ""
  "PORT\r\n"
  "TYPE=ASYNC\r\n"
  "ID=KISS Serial1\r\n"
  "COMPORT=Serial1\r\n"
  "SPEED=57600\r\n"
  "QUALITY=0\r\n"
  "ENDPORT\r\n";

//char apn[] = "iot.1nce.net";

/*
For data / GPRS:

Name: 1pMobile
APN: data.uk
User: user
Password: one2one
*/

extern "C" char CONSOLELOG[2048];
extern "C" char DEBUGLOG[2048];

int consoleloglen = 0;
int debugloglen = 0;

void getBoardConfig() {

  int n = 0;
  char Line[512];
  char *Param, *Context;

  char *Value;

  if (cfgOpen((char *)"board.cfg", (char *)"r") == 0) 
  {
    Serial.printf("Can't read board.cfg - using defaults");
    return;
  }

  while (1) 
  {
    n = cfgRead((unsigned char *)Line, 512);

    if (n == 0) 
    {
      cfgClose();
      return;
    }

    strlop(Line, ';');

    Param = strtok_r(Line, "= ", &Context);
    Value = strtok_r(NULL, "= ", &Context);

    if (strcmp(Param, "NodeCall") == 0)
      strlcpy(DefaultNodeCall, Value, 10);

    if (strcmp(Param, "DebugPort") == 0) 
    {
      DEBUG = GetSerialDevice(Value);

      if (DEBUG == nullptr) {
        Serial.println("Invalid DEBUG port");
        DEBUG = (HardwareSerial *)&Serial;
      }
    }

    if (strcmp(Param, "Console") == 0) 
    {
      CONSOLE = GetSerialDevice(Value);

      if (CONSOLE == nullptr) {
        Serial.println("Invalid CONSOLE port");
        CONSOLE = (HardwareSerial *)&Serial;
      }
    }

    if (strcmp(Param, "RTC") == 0) 
    {
      if (strcmp(Value, "DS1904") == 0)
        RTC = RTCDS1904;
      else if (strcmp(Value, "DS1307") == 0)
        RTC = RTCDS1307;
      else if (strcmp(Value, "PCF8523") == 0)
        RTC = RTCPCF8523;
    }

    if (strcmp(Param, "SMSDEST") == 0)
      strlcpy(SMSDEST, Value, 16);

    if (strcmp(Param, "SMSEvents") == 0) 
    {
      if (strstr(Value, "Restart"))
        SMSEvents |= SMSonRestart;
      if (strstr(Value, "CallFailed"))
        SMSEvents |= SMSonCallFailed;
    }
 
    if (strcmp(Param, "Callback1") == 0)
      strlcpy(ConnectOut1, Value, 63);

    if (strcmp(Param, "Callback2") == 0)
      strlcpy(ConnectOut2, Value, 63);

    if (strcmp(Param, "Clipper") == 0)
      useClipper = 1;
  }
}




HardwareSerial *GetSerialDevice(char *NameVal) 
{
  HardwareSerial *Device = nullptr;

  // Now just gets a device from a name. Doesn't mess with params as that seems to upset some devices
  // So speed if present is ignored

  // If name includes a : then speed is specified

  char Name[32];
  int Speed;

  strncpy(Name, NameVal, 32);
  Speed = atoi(strlop(Name, ':'));

  // Serial,  Serial1 and Serial 2 are always defined. Serial is normally a USB device

  if (strcmp(Name, "Serial") == 0)
    Device = (HardwareSerial *)&Serial;
  else if (strcmp(Name, "Serial1") == 0)
    Device = &Serial1;
  else if (strcmp(Name, "Serial2") == 0)
    Device = &Serial2;
  else if (strcmp(Name, "Serial3") == 0)
    Device = Serial3;
  else if (strcmp(Name, "Serial4") == 0)
    Device = Serial4;
  else if (strcmp(Name, "Serial5") == 0)
    Device = Serial5;

#ifdef USE_TINYUSB
  else if (CFG_TUD_CDC > 1 && strcmp(Name, "USBSer1") == 0)
    Device = (HardwareSerial *)&USBSer1;
  else if (CFG_TUD_CDC > 2 && strcmp(Name, "USBSer2") == 0)
    Device = (HardwareSerial *)&USBSer2;
#endif
  else {
    Consoleprintf("Unknown port name %s", Name);
    return 0;
  }

  //  Device->setTimeout(1);

  //  if (Speed)
  //   Device->begin(Speed);

  return Device;
}

extern "C" void sendMQTTStatus(const char *Status) 
{
  char Line[512];
  char Msg[256];
  int len;

  if (useClipper == 0 || MQTT_HOST[0] == 0)
    return;

  LTESerial->printf("AT+CMQTTDISC=0,120\r\n");
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTREL=0\r\n");
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTSTOP\r\n");
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTSTART\r\n");
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTACCQ=0,\"%s\"\r\n", NODECALLLOPPED);
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTCONNECT=0,\"tcp://%s:%d\",60,1,\"%s\",\"%s\"\r\n", MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
  LTEReadAll(Line, 511, 500);

  LTEReadAll(Line, 511, 500);

  len = sprintf(Msg, "PACKETNODE/%s", NODECALLLOPPED);
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTTOPIC=0,%d\r\n", len);
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("%s\r\n", Msg);
  LTEReadAll(Line, 511, 500);

  len = sprintf(Msg, "{\"status\":\"%s\"}", Status);

  LTESerial->printf("AT+CMQTTPAYLOAD=0,%d\r\n", len);
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("%s\r\n", Msg);
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTPUB=0,1,60\r\n");

  LTEReadAll(Line, 511, 500);
  if (strstr(Line, "+CMQTT")) OutputDebugString(Line);

  LTEReadAll(Line, 511, 500);
  if (strstr(Line, "+CMQTT")) OutputDebugString(Line);

  LTEReadAll(Line, 511, 500);
  if (strstr(Line, "+CMQTT")) OutputDebugString(Line);

  LTESerial->printf("AT+CMQTTDISC=0,120\r\n");
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTREL=0\r\n");
  LTEReadAll(Line, 511, 500);

  LTESerial->printf("AT+CMQTTSTOP\r\n");
  LTEReadAll(Line, 511, 500);
  if (strstr(Line, "+CMQTT")) OutputDebugString(Line);
}

extern "C" void resetClipper() 
{
  digitalWrite(RESETPIN, LOW);
  FastPollDelay(100);
  digitalWrite(RESETPIN, HIGH);
}

extern "C" void initClipper() 
{
  int ret = 0;
  int n = 10;
  char Line[512];
  char *ptr;

  pinMode(PWRPIN, OUTPUT);
  pinMode(RESETPIN, OUTPUT);

  digitalWrite(RESETPIN, HIGH);
  digitalWrite(PWRPIN, HIGH);

  FastPollDelay(100);
  digitalWrite(RESETPIN, LOW);
  FastPollDelay(100);
  digitalWrite(RESETPIN, HIGH);

  CONTERM[2] = LTESerial;  // LTE Modem

  // For 1NCE AT+CGAUTH=1,1
  //AT+CGDCONT=1,"IP","iot.1nce.net"


  //AT+CGAUTH=1,2,"one2one","user"
  //AT+CGDCONT=1,"IP","data.uk"

  LTESerial->println("ATE0");            // echo off
  int Len = LTEReadAll(Line, 511, 500);  // ATE0 Response

  if (Len == 0) 
  {
    Debugprintf("Clipper not reponding - disabling");
    useClipper = 0;
    if (RTC == RTCLTE)
      RTC = RTCNone;

    return;
  }

  LTESerial->println("AT+CIPMODE=1");  // TCP Transparent mode
  LTEReadAll(Line, 511, 1000);         // CIP Response
  LTESerial->println("AT+NETOPEN");    // Need to wait for NETOPEN response
  Debugprintf("Waiting for NETOPEN Response");
  LTEReadAll(Line, 511, 5000);  // Get OK from NETOPEN
  Debugprintf("Checking NETOPEN Response");
  if (strstr(Line, "+NETOPEN:"))
    Debugprintf("NETOPEN Ok");
  else {
    LTEReadAll(Line, 511, 5000);  // Get OK from NETOPEN
    if (strstr(Line, "+NETOPEN:"))
      Debugprintf("NETOPEN Ok");
  }

  LTESerial->println("AT+IPADDR");
  LTESerial->println("AT+CCLK?");
  LTESerial->println("AT+CMGF=1");  // SMS Text Mode
  LTEReadAll(Line, 511, 5000);

  // if (RTC == RTCLTE)
  {
    if (ptr = strstr(Line, "+CCLK:")) {
      // "24/11/13,17:22:51

      time_t tSet;
      struct tm tm;
      struct timeval now;

      ptr += 8;

      tm.tm_year = 10 * (ptr[0] - '0') + ptr[1] - '0' + 100;
      tm.tm_mon = 10 * (ptr[3] - '0') + ptr[4] - '0' - 1;
      tm.tm_mday = 10 * (ptr[6] - '0') + ptr[7] - '0';
      tm.tm_hour = 10 * (ptr[9] - '0') + ptr[10] - '0';
      tm.tm_min = 10 * (ptr[12] - '0') + ptr[13] - '0';
      tm.tm_sec = 10 * (ptr[15] - '0') + ptr[16] - '0';

      now.tv_sec = mktime(&tm);
      now.tv_usec = 0;

      settimeofday(&now, NULL);

      time_t szClock = time(NULL);
      struct tm *TM = gmtime(&szClock);

      Consoleprintf("Time from Clipper %s, %02d %s %3d %02d:%02d:%02d GMT",
                    dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon], TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);
    }
  }
  if (ptr = strstr(Line, "+IPADDR:")) {
    strlop(ptr, 13);
    Debugprintf("IP Address %s", ptr + 9);
  }
}


extern "C" void sendSMS(char *Number, const char *text) {
  char Line[256];

  if (useClipper == 0)
    return;

  LTESerial->printf("AT+CMGS=\"%s\"\r\n", Number);
  LTEReadAll(Line, 255, 1000);
  Debugprintf(Line);
  LTESerial->printf("From %s %s\r\n%c", NODECALLLOPPED, text, 26);  // ctrl/z on end
  LTEReadAll(Line, 255, 1000);
  Debugprintf(Line);
}

uint32_t DS1904getClock() 
{
  uint32_t Time;

  oneWire_reset(OWPIN);

  oneWire_write(0xcc, OWPIN);  // Skip addr
  oneWire_write(0x66, OWPIN);  // read RTC

  Time = oneWire_read(OWPIN);  // read and discard control byte
  Time = oneWire_read(OWPIN);
  Time |= oneWire_read(OWPIN) << 8;
  Time |= oneWire_read(OWPIN) << 16;
  Time |= oneWire_read(OWPIN) << 24;

  return Time;
}

extern "C" void DS1904setClock(uint32_t Time) 
{
  oneWire_reset(OWPIN);

  oneWire_write(0xcc, OWPIN);  // Skip addr
  oneWire_write(0x99, OWPIN);  // write RTC - this is the write code
  oneWire_write(0xAC, OWPIN);  //This is the control byte.  AC in hex = 10101100


  oneWire_write(Time & 0xff, OWPIN);
  oneWire_write((Time >> 8) & 0xff, OWPIN);
  oneWire_write((Time >> 16) & 0xff, OWPIN);
  oneWire_write((Time >> 24) & 0xff, OWPIN);

  oneWire_reset(OWPIN);
}


extern "C" void picocreateDefaultcfg() 
{
  cfg = LittleFS.open("bpq32.cfg", "w");

  if (cfg) {
    cfg.write(dummyConfig, strlen(dummyConfig));
    cfg.close();
  }
}


uint32_t nextTimerRun = 0;
uint64_t HourTimerMicros = 0;
uint64_t SixHourTimerMicros = 0;



void setup() 
{
  char Line[512];

  LittleFS.begin();

#ifdef USE_TINYUSB
  if (CFG_TUD_CDC > 1) USBSer1.begin(115200);
  if (CFG_TUD_CDC > 2) USBSer2.begin(115200);
#endif

  Serial.setTimeout(1);
  Serial.begin(115200);

  Serial1.setTimeout(1);
  Serial1.begin(115200);

  Serial2.setTimeout(1);
  Serial2.begin(115200);

  getBoardConfig();

#ifdef USE_TINYUSB
  Debugprintf("Using Adafruit TINYUSB. CFG_TUD_CDC = %d", CFG_TUD_CDC);
 #endif
  
  Serial3 = new SerialPIO(SerTX[2], SerTX[2] + 1, 256);
  Serial4 = new SerialPIO(SerTX[3], SerTX[3] + 1, 256);
  Serial5 = new SerialPIO(SerTX[4], SerTX[4] + 1, 256);
  Serial6 = new SerialPIO(SerTX[5], SerTX[5] + 1, 256);

  LTESerial = Serial6;
  LTESerial->setTimeout(1);
  LTESerial->begin(115200);

  int n = 10;
  while (!Serial && n--)
    delay(1000);

  Serial.println("Starting");

  // check to see if multiple CDCs are enabled
#ifdef USE_TINYUSB
  if (CFG_TUD_CDC < 2) {
    Serial.printf("To use multiple USB Serial ports CFG_TUD_CDC must be at least 2, current value is %u\n", CFG_TUD_CDC);
    Serial.println("Config file is tusb_config_rp2040.h. Search for it - location can change");
    Serial.println("Continuing with one usb serial device (Serial)");
  }
#endif

  // These may be changed from board.cfg but sensible to set here so Debugprintf works in config processing

  CONSOLE = GetSerialDevice(ConsolePort);
  DEBUG = GetSerialDevice(DebugPort);
  CONTERM[0] = (HardwareSerial *)&Serial;

#ifdef USE_TINYUSB
  if (CFG_TUD_CDC > 1)
    CONTERM[1] = (HardwareSerial *)&USBSer1;
 #endif

  Consoleprintf("Starting");

  // check to see if multiple CDCs are enabled
#ifdef USE_TINYUSB
  if (CFG_TUD_CDC < 2) {
    Consoleprintf("To use multiple USB Serial ports CFG_TUD_CDC must be at least 2, current value is %u", CFG_TUD_CDC);
    Consoleprintf("Config file is tusb_config_rp2040.h. Search for it - location can change");
    Consoleprintf("Continuing with one usb serial device (Serial)");
  }
#endif

  if (RTC == RTCDS1904) {
    digitalWrite(OWPIN, LOW);
    pinMode(OWPIN, INPUT);
  }

  if (Consolenames[0][0])
    CONTERM[0] = GetSerialDevice(Consolenames[0]);
  if (Consolenames[1][0])
    CONTERM[1] = GetSerialDevice(Consolenames[1]);
  // if (Consolenames[2][0])
  //   CONTERM[2] = GetSerialDevice(Consolenames[2]);

  if (useClipper)
    delay(5000);

  rp2040.wdt_begin(8300);

  struct timeval secs;
  int rc;

  secs.tv_sec = getRTC();
  secs.tv_usec = 0;
  rc = settimeofday(&secs, NULL);

  time_t szClock = time(NULL);
  struct tm *TM = gmtime(&szClock);

  Consoleprintf("Time from RTC %s, %02d %s %3d %02d:%02d:%02d GMT",
                dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon], TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

  if (useClipper) 
  {
    initClipper();

    if (useClipper == 0) 
    {
      useClipper = 1;
      delay(5);
      initClipper();
    }
  }

  BPQInit();

  Consoleprintf("Init Complete at %s, %02d %s %3d %02d:%02d:%02d GMT",
                dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon], TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

  RP2040::resetReason_t rr = rp2040.getResetReason();

  char resetReasonText[][24] = { "Unknown", "Power On / Brownout", "Run pin", "Software", "Watchdog Timer", "Debug reset" };

  Consoleprintf("Reset Reason %s", resetReasonText[rr]);

  Serial.println("************* Debug Log ******************");
  Serial.print(DEBUGLOG);
  Serial.println("************* Console Log ******************");
  Serial.print(CONSOLELOG);
  Serial.println("*******************************************");

  LTEReadAll(Line, 511, 500);  // Clear LTE Read
  if (Line[0])
    Debugprintf(Line);

  sendMQTTStatus("Restarting");

  HourTimerMicros = time_us_64();
  SixHourTimerMicros = time_us_64();  // Offset timers by a minute

  //
  if (SMSEvents & SMSonRestart)
    sendSMS(SMSDEST, "picoNode restarted");
}
extern "C" int sendtoDialSession(char *Line, int rxed);

int LTEReadAll(char *Line, int len, int mS) 
{
  // reads characters until they stop or for mS
  int start = millis();
  int end = millis() + mS;
  int ret = 0, c;

  rp2040.wdt_begin(8300);

  // wait for first

  while (end > millis()) 
  {

    if (LTESerial->available()) 
    {
      // Got first

      c = LTESerial->read();

      Line[ret++] = c;

      // now read until no more available

      while (1) 
      {
        delayMicroseconds(200);
        rp2040.wdt_begin(8300);

        BPQFastPoll();

        if (LTESerial->available() == 0 || ret >= len) 
        {
          // no more
          Line[ret] = 0;
          return (ret);
        }

        c = LTESerial->read();
        Line[ret++] = c;
      }
    }
    BPQFastPoll();
  }

  Line[ret] = 0;
  return 0;
}

void FastPollDelay(int mS) 
{
  int end = millis() + mS;

  rp2040.wdt_begin(8300);

  while (end > millis()) 
  {
    delay(1);
    BPQFastPoll();
  }
}

int tcpConnected = 0;
int tcpConnecting = 0;

extern "C" void tcpCall(char *host, int port) 
{
  // AT+CIPOPEN=0,"TCP","NOTTM.g8bpq.net", 8015

  LTESerial->printf("AT+CIPOPEN=0,\"TCP\",\"%s\", %d\r\n", host, port);

  tcpConnecting = 1;
}

void pollLTE() {
  // See if any input from LTE modem.

  if (useClipper == 0)
    return;

  char Line[512];
  int Len, ret;

  // if notification of a text message, get message and action
  // if attached to a terminal (dial command) send to user
  // if attached to a ConTerm, send to Node.
  // Otherwise write to debug log

  if (LTESerial->available()) 
  {
    ret = LTESerial->readBytesUntil('\n', Line, 511);
    Line[ret] = 0;

    if (ret) 
    {
      // Check for Text Message (+CMT: "+447760401072","","24/11/11,16:38:25+0")

      if (memcmp(Line, "+CMT: ", 6) == 0) 
      {
        int ret = LTEReadAll(Line, 511, 500);
        char host[256];

        Debugprintf("SMS %d, %s", ret, Line);

        // Get host to call

        if (stristr(Line, (char *)"Callback2"))
          strcpy(host, ConnectOut2);
        else
          strcpy(host, ConnectOut1);

        Debugprintf("Calling %s", host);

        char *port = strlop(host, ':');

        tcpCall(host, atoi(port));
        return;
      }

      if (tcpConnecting) 
      {
        // look for CONNECT 115200 or CONNECT FAIL or ERROR

        if (strstr(Line, "CONNECT FAIL")) 
        {
          tcpConnecting = 0;
          Debugprintf("Connect Failed");
        } 
        else if (strstr(Line, "CONNECT 115200")) 
        {
          tcpConnecting = 0;
          tcpConnected = 1;
          Debugprintf("TCP Connected");
          LTESerial->printf("%s\r", NODECALLLOPPED);
        } 
        else if (strstr(Line, "ERROR")) 
        {
          // try reinitialising

          tcpConnecting = 0;
          tcpConnected = 0;
          initClipper();
        }
      }

      if (tcpConnected) 
      {
        // send to node command handler

        ConTermInput(2, Line);
        return;
      }


      if (sendtoDialSession(Line, ret))
        return;

      // Default
      Debugprintf("%s", Line);
    }
  }
}

const uint64_t HourMicroSecs = 3600 * 1000 * 1000LLU;
const uint64_t SixHoursMicroSecs = 6 * 3600 * 1000 * 1000LLU;

void loop() 
{
  rp2040.wdt_begin(8300);
  delay(1);

  BPQFastPoll();  // Run every mS to reduce chance of serial port overrun

  pollLTE();

  if (millis() > nextTimerRun)  // Run timer every 100 mS
  {
    nextTimerRun = millis() + 100;
    BPQTimerLoop();

    uint64_t NowMicros = time_us_64();

    if ((NowMicros - HourTimerMicros) > HourMicroSecs) 
    {
      HourTimerMicros += HourMicroSecs;
      Debugprintf("Hour");
      sendMQTTStatus("online");
    }

    if ((NowMicros - SixHourTimerMicros) > SixHoursMicroSecs) 
    {
      SixHourTimerMicros += SixHoursMicroSecs;
      Debugprintf("6 Hours");
      SaveNodes();
      SaveMH();
    }
  }
}

extern "C" void OutputDebugString(char *Mess) 
{
  // Save in buffer then send to device if defined

  int len = strlen(Mess);

  if ((len + debugloglen) > 2046) 
  {
    int removed = (len + debugloglen) - 2046;
    memmove(DEBUGLOG, &DEBUGLOG[removed], debugloglen - removed + 1);
    debugloglen -= removed;
  }

  strcat(DEBUGLOG, Mess);
  debugloglen += len;

  if (DEBUG)
    DEBUG->print(Mess);
}

extern "C" int WritetoConsoleLocal(char *Mess)
{
  // Save in buffer then send to device if defined

  int len = strlen(Mess);

  if ((len + consoleloglen) > 2046) 
  {
    int removed = (len + consoleloglen) - 2046;
    memmove(CONSOLELOG, &CONSOLELOG[removed], consoleloglen - removed + 1);
    consoleloglen -= removed;
  }

  strcat(CONSOLELOG, Mess);
  consoleloglen += len;

  if (CONSOLE)
    CONSOLE->print(Mess);

  return 0;
}

extern "C" void Debugprintf(const char *format, ...) 
{
  char Mess[5000];
  va_list(arglist);
  va_start(arglist, format);
  vsnprintf(Mess, 4097, format, arglist);
  strcat(Mess, "\r\n");
  OutputDebugString(Mess);

  return;
}

extern "C" int Consoleprintf(const char *format, ...) 
{
  char Mess[5000];
  va_list(arglist);
  va_start(arglist, format);
  vsnprintf(Mess, 4097, format, arglist);
  strcat(Mess, "\r\n");

  // Save in buffer then send to device if defined

  int len = strlen(Mess);

  if ((len + consoleloglen) > 2046) 
  {
    int removed = (len + consoleloglen) - 2046;
    memmove(CONSOLELOG, &CONSOLELOG[removed], consoleloglen - removed + 1);
    consoleloglen -= removed;
  }

  strcat(CONSOLELOG, Mess);
  consoleloglen += len;

  if (CONSOLE)
    CONSOLE->print(Mess);

  return 0;
}

extern "C" void Contermprint(int n, char *Msg) 
{
  if (CONTERM[n])
    CONTERM[n]->print(Msg);

  return;
}


extern "C" uint64_t GetTickCount()
{
  return millis();
}


extern "C" void Sleep(int mS) {
  delay(mS);
}

char *xxConfig;
int n;
char *rest;

extern "C" char *strlop(char *buf, char delim);

extern "C" int cfgOpen(char *FN, char *Mode) {
  Consoleprintf("Open cfg %s", FN);

  cfg = LittleFS.open(FN, Mode);
  if (!cfg) {
    return 0;
  }

  xxConfig = (char *)malloc(8192);
  int n = cfg.read((unsigned char *)xxConfig, 8190);

  xxConfig[n] = 0;
  rest = xxConfig;

  cfg.close();
  return 1;
}

extern "C" int cfgClose() {
  free(xxConfig);
  return 0;
}

extern "C" int cfgRead(unsigned char *Buffer, int Len) {
  // return a line at a time

  if (rest == 0 || rest[0] == 0)
    return 0;

  char *p = strlop(rest, '\r');

  strcpy((char *)Buffer, rest);

  int n = strlen(p);

  rest = p;

  if (rest && rest[0] == '\n')
    rest++;

  return n;
}


extern "C" int ConTermKbhit(int n) {
  if (CONTERM[n])
    return CONTERM[n]->read();

  return -1;
}


extern "C" int getFreeHeap() {
  return rp2040.getFreeHeap();
}

// Serial Port Support

// Serial1 (0,1) SerialPIO (2,3 4,5 6,7) Serial2 (8,9) SerialPIO (10,11)

#define maxHandles 8

HardwareSerial *Handles[maxHandles] = { 0 };

extern "C" int picoOpenSerial(char *Name, int speed) {
  // Find a free handle

  int i;

  for (i = 0; i < maxHandles; i++) {
    if (Handles[i] == 0)
      break;
  }

  if (i == 8) {
    DEBUG->println("No Serial Handles free");
    return 0;
  }

  if (strcmp(Name, "Serial1") == 0) {
    Serial1.setFIFOSize(256);
    Handles[i] = &Serial1;
  } else if (strcmp(Name, "Serial2") == 0) {
    Serial2.setFIFOSize(256);
    Handles[i] = &Serial2;
  } else if (strcmp(Name, "Serial3") == 0)
    Handles[i] = Serial3;
  else if (strcmp(Name, "Serial4") == 0)
    Handles[i] = Serial4;
  else if (strcmp(Name, "Serial5") == 0)
    Handles[i] = Serial5;
#ifdef USE_TINYUSB
  else if (CFG_TUD_CDC > 1 && strcmp(Name, "USBSer1") == 0) {
    Handles[i] = (HardwareSerial *)&USBSer1;
    Handles[i]->setTimeout(1);
    return i + 1;
  } else if (CFG_TUD_CDC > 2 && strcmp(Name, "USBSer2") == 0) {
    Handles[i] = (HardwareSerial *)&USBSer2;
    Handles[i]->setTimeout(1);
    return i + 1;
  }
#endif
  else {
    DEBUG->printf("Unknown port name %s", Name);
    return 0;
  }

  Handles[i]->setTimeout(1);
  Handles[i]->begin(speed);

  return i + 1;
}

extern "C" int picoWriteDial(char *Buffer, int Len) {
  // Need to check available. Looks like it only returns 1/0 not actual space so have to check for each byte

  for (int n = 0; n < Len; n++) {
    while (LTESerial->availableForWrite() == 0) {
      BPQFastPoll();
    }
    LTESerial->write(Buffer[n]);
  }
  return n;
}
extern "C" int picoCloseSerial(int Chan) {
  return 0;
}

extern "C" int picoWriteSerial(int Chan, char *Buffer, int Len) {
  if (Chan == 0)
    return 0;

  Chan--;

  // Need to check available. Looks like it only returns 1/0 not actual space so have to check for each byte

  for (int n = 0; n < Len; n++) {
    while (Handles[Chan]->availableForWrite() == 0) {
      BPQFastPoll();
    }
    Handles[Chan]->write(Buffer[n]);
  }
  return Len;
}

extern "C" int picoReadSerial(int Chan, char *Buffer, int Len) {
  int ptr = 0;
  int ret;

  if (Chan == 0)
    return 0;

  Chan--;

  if (Handles[Chan]->available() && ptr < Len) {
    ret = Handles[Chan]->read();
    Buffer[ptr++] = ret;
  }

  return ptr;
}

extern "C" int picoSerialGetLine(int Chan, char *Line, int maxline) {
  int ret = 0;

  if (Chan == 0)
    return 0;

  Chan--;

  if (Handles[Chan]->available()) {
    ret = Handles[Chan]->readBytesUntil('\n', Line, maxline);
    Line[ret] = 0;
  }
  return ret;
}



extern "C" int DoRoutes();
extern "C" int DoNodes();

extern "C" int SaveNodes() {
  cfg = LittleFS.open("BPQNODES.dat", "w");

  if (!cfg)
    return 0;

  DoRoutes();
  DoNodes();

  cfg.close();
  return (0);
}

extern "C" void writeNodesLine(char *line) {
  cfg.write(line, strlen(line));
}

File root;
File file;

extern "C" void OpenDirectory() {
  root = LittleFS.open("/", "r");
  file = root.openNextFile();
}

extern "C" void GetNextDirEntry(char *line) {
  if (file) {
    if (file.isDirectory())
      sprintf(line, "  DIR : %s", file.name());
    else
      sprintf(line, ("  FILE: %s\tSIZE: %d"), file.name(), file.size());

    file = root.openNextFile();
    return;
  }

  line[0] = 0;
  file.close();
  root.close();
  return;
}




extern "C" int picoDelete(char *from) {
  return LittleFS.remove(from);
}



extern "C" int picoRename(char *from, char *to) {
  return LittleFS.rename(from, to);
}


extern "C" File *picoOpenFile(char *FN) {
  if (!LittleFS.exists(FN))
    return 0;

  File *file = 0;
  file = new (File);

  *file = LittleFS.open(FN, "r");

  return file;
}

extern "C" int picoGetLine(File *file, char *Line, int maxline) {
  int n = file->readBytesUntil('\n', Line, maxline);
  Line[n] = 0;

  return n;
}

extern "C" void picoCloseFile(File *file) {
  file->close();
  delete file;
}

extern "C" int GetNextFileBlock(File *file, char *line) {
  int n = file->read((unsigned char *)line, 128);
  line[n] = 0;

  return n;
}


extern "C" File *picoCreateFile(char *FN) {
  File *file = 0;
  file = new (File);

  *file = LittleFS.open(FN, "w");

  return file;
}




extern "C" void picofprintf(File *file, const char *format, ...) {
  char Mess[1000];
  va_list(arglist);
  va_start(arglist, format);
  int n = vsnprintf(Mess, 1000, format, arglist);

  file->write(Mess, n);
}

extern "C" void picoWriteLine(File *file, char *Data, int len) {
  file->write(Data, len);
}

extern "C" int createandwritefile(char *FN, char *Data, int len) {
  File file = LittleFS.open(FN, "w");

  if (file) {
    file.write(Data, len);
    file.close();

    return 1;
  }

  return 0;  // open failed
}


uint32_t getRTC() 
{
  DateTime now;

  if (RTC == RTCDS1307) 
  {
    rtc = new (RTC_DS1307);

    Wire.setSDA(4);
    Wire.setSCL(5);
    Wire.begin();

    if (!rtc->begin(&Wire)) 
    {
      Serial.println("Couldn't find RTC");
      Serial.flush();
    }

    if (!rtc->isrunning()) 
    {
      Serial.println("RTC is NOT running, setting time to compile time");
      rtc->adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    now = rtc->now();
    return now.unixtime();
  }

  if (RTC == RTCPCF8523) 
  {
    rtcpcf = new (RTC_PCF8523);

    Wire.setSDA(4);
    Wire.setSCL(5);
    Wire.begin();

    if (!rtcpcf->begin(&Wire)) 
    {
      Serial.println("Couldn't find RTC");
      Serial.flush();
    }

    if (!rtcpcf->initialized() || rtcpcf->lostPower()) 
    {
      Serial.println("RTC is NOT initialized, setting time to compile time");
      rtcpcf->adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    now = rtcpcf->now();
    return now.unixtime();
  }

  if (RTC == RTCDS1904)
    return DS1904getClock();

  // No RTC. Default to compile time

  rtcm = new (RTC_Millis);
  rtcm->begin(DateTime(F(__DATE__), F(__TIME__)));
  now = rtcm->now();
  return now.unixtime();
}

extern "C" void setRTC(uint32_t Time) 
{
  if (RTC == RTCDS1307) 
  {
    rtc->adjust(DateTime(Time));
    return;
  }

  if (RTC == RTCPCF8523) 
  {
    rtcpcf->adjust(DateTime(Time));
    return;
  }

  if (RTC == RTCDS1904)
    return DS1904setClock(Time);
}
