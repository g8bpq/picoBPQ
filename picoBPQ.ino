#include "microOneWire.h"
#include "LittleFS.h" // LittleFS is declared
#include "Adafruit_TinyUSB.h"

#define OWPIN 15    // Pin for DS1904 RTC

Adafruit_USBD_CDC USBSer1;
Adafruit_USBD_CDC USBSer2;
Adafruit_USBD_CDC USBSer3;

// Serial Port Support

// Serial1 (0,1) SerialPIO (2,3 4,5 6,7) Serial2 (8,9) SerialPIO (10,11)

SerialPIO * Serial3;
SerialPIO * Serial4;
SerialPIO * Serial5;
SerialPIO * Serial6;

extern "C" void BPQTimerLoop();
extern "C" void BPQInit();
extern "C" void BPQFastPoll();

File cfg;

const char dummyConfig[] = 
  "SIMPLE\r\n"
  "NODECALL=DUMMY\r\n"
  "NODEALIAS=RP2040\r\n"
  "PASSWORD=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n"
  ""
  "PORT\r\n"
  "TYPE=INTERNAL\r\n"
  "ENDPORT\r\n"
  ""
  "PORT\r\n"
  "TYPE=ASYNC\r\n"
  "ID=KISS Serial1\r\n"
  "COMPORT=Serial1\r\n"
  "SPEED=57600\r\n"
  "QUALITY=0\r\n"
  "ENDPORT\r\n";


Adafruit_USBD_CDC * CONSOLE;
Adafruit_USBD_CDC * CONTERM[4];
Adafruit_USBD_CDC * DEBUG;


uint32_t DS1904getClock()
{
  uint32_t Time;

  oneWire_reset(OWPIN);

  oneWire_write(0xcc, OWPIN);    // Skip addr
  oneWire_write(0x66, OWPIN);   // read RTC

  oneWire_read(OWPIN);      // read and discard control byte
  Time = oneWire_read(OWPIN);
  Time |= oneWire_read(OWPIN) << 8;
  Time |= oneWire_read(OWPIN) << 16;
  Time |= oneWire_read(OWPIN) << 24;
  
  return Time;
}

extern "C" void DS1904setClock(uint32_t Time)
{
  oneWire_reset(OWPIN);

  oneWire_write(0xcc, OWPIN);    // Skip addr
  oneWire_write(0x99, OWPIN);   // write RTC - this is the write code
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

  if (cfg)
  {
    cfg.write(dummyConfig, strlen(dummyConfig));
    cfg.close();
  }
}
 

void setup() 
{
  // put your setup code here, to run once:

  LittleFS.begin();

  Serial.begin(115200);

  while (!Serial)
    delay(1000);

  // check to see if multiple CDCs are enabled
  if (CFG_TUD_CDC < 2 )
  {
    Serial.printf("To use multiple USB Serial ports CFG_TUD_CDC must be at least 2, current value is %u\n", CFG_TUD_CDC);
    Serial.println("Config file is located in arduino15/packages/rp2040/hardware/rp2040/4.2.0/libraries/Adafruit_TinyUSB_Arduino/src/arduino/ports/rp2040/tusb_config_rp2040.h");
    Serial.println("Continuing with one usb serial device");
  }

  USBSer1.begin(115200);
  USBSer2.begin(115200);
  USBSer3.begin(115200);

  CONSOLE = &Serial;
  
  DEBUG = &Serial; 
  
  CONTERM[0] = &Serial;
  CONTERM[1] = &USBSer1;
  CONTERM[2] = &USBSer2;

  struct timeval now;
  int rc;

  now.tv_sec=DS1904getClock();
  now.tv_usec=0;

  rc=settimeofday(&now, NULL);

  //CONSOLE->printf("Starting. Last reset caused by %d", rp2040.
  CONSOLE->printf("Starting\n");

//UNKNOWN_RESET, PWRON_RESET, RUN_PIN_RESET, SOFT_RESET, WDT_RESET, DEBUG_RESET, GLITCH_RESET, BROWNOUT_RESET};

  rp2040.wdt_begin(8300);

  BPQInit();
  CONSOLE->print("Init Complete RTC Time ");
  Serial.println(DS1904getClock());

  Serial.printf("CFG_TUD_CDC %d\n",CFG_TUD_CDC);
//  Serial.printf("DTR %d\n", USBSer2.dtr());

}


uint32_t nextTimerRun = 0;

void loop() 
{
  rp2040.wdt_begin(8300);
  delay(1);

  BPQFastPoll();   // Run every mS to reduce chance of serial port overrun 

  if (millis() > nextTimerRun)    // Runtimer every 100 mS
  {
    nextTimerRun = millis() + 100;
    BPQTimerLoop();
  }
}

extern "C" void OutputDebugString(char * Mess)
{
  CONSOLE->print(Mess);
}

extern "C" int WritetoConsoleLocal(char * buff)
{
	return CONSOLE->printf("%s", buff);
}

extern "C" void Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);
	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

extern "C" void Contermprint(int n, char * Msg)
{
  CONTERM[n]->print(Msg);
	return;
}





extern "C" uint64_t GetTickCount()
{
	return millis();
}



extern "C" void Sleep(int mS)
{
  delay(mS);
}

char * xxConfig;
int n;
char * rest;

extern "C" char * strlop(char * buf, char delim);

extern "C" int cfgOpen(char * FN, char * Mode)
{
   CONSOLE->printf("Open cfg %s\r\n", FN);

  cfg = LittleFS.open(FN, Mode);
  if (!cfg)
 {
     CONSOLE->println("file open failed");
    return 0;
 }

 xxConfig = (char *)malloc(8192);
 int n = cfg.read((unsigned char *)xxConfig, 8190);

  xxConfig[n] = 0;

  rest = xxConfig;

  cfg.close();

 return 1;

}
extern "C" int cfgClose()
{
  free(xxConfig);
  return 0;
}

extern "C" int cfgRead(unsigned char * Buffer, int Len)
{
  // return a line at a time

  if (rest == 0 || rest[0] == 0)
    return 0;

  char * p = strlop(rest, '\r');

  strcpy((char *)Buffer, rest);

  int n = strlen(p);

  rest = p;

  if (rest && rest[0] == '\n')
    rest++;

 return n;
}


extern "C" int ConTermKbhit(int n)
{
  return CONTERM[n]->read();
}


extern "C" int getFreeHeap()
{
  return rp2040.getFreeHeap();
}

// Serial Port Support

// Serial1 (0,1) SerialPIO (2,3 4,5 6,7) Serial2 (8,9) SerialPIO (10,11)

#define maxHandles 8

HardwareSerial * Handles[maxHandles] = {0};
Adafruit_USBD_CDC * USBHandles[maxHandles] = {0};
int isUSB[maxHandles] = {0};



extern "C" int picoOpenSerial(char * Name, int speed)
{
  // Find a free handle

  int i;
  
  for (i = 0; i < maxHandles; i++)
  {
    if (Handles[i] == 0 && USBHandles[i] == 0)
      break;
  }

  if (i == 8)
  {
    DEBUG->println("No Serial Handles free");
    return 0;
  }

  if (strcmp(Name, "Serial1") == 0)
  {
    Serial1.setFIFOSize(256);
    Handles[i] = &Serial1;
  }
  else if (strcmp(Name, "Serial2") == 0)
  {
  //  Serial2.setFIFOSize(1024);
    Handles[i] = &Serial2;
  }
  else if (strcmp(Name, "Serial3") == 0)
  {
    Serial3 = new SerialPIO(2, 3, 256);
    Handles[i] = Serial3;
  }
   else if (strcmp(Name, "Serial4") == 0) 
   {
    Serial4 = new SerialPIO(4, 5, 256);
    Handles[i] = Serial4;
  }
  else if (strcmp(Name, "Serial5") == 0)  
  {
    Serial5 = new SerialPIO(6, 7, 256);
    Handles[i] = Serial5;
  }
  else if (strcmp(Name, "Serial6") == 0)  
  {
    Serial6 = new SerialPIO(10, 11, 256);
    Handles[i] = Serial6;
  }

 else if (strcmp(Name, "USBSer2") == 0)
  {
    USBHandles[i] = &USBSer2;
    isUSB[i] = 1;
  }
  else
  {
    DEBUG->printf("Unknown port name %s\n", Name);
    return 0;
  }

  if (isUSB[i])
    USBHandles[i]->begin(speed);
  else
  {
    Handles[i]->begin(speed);
  }

  return i + 1;
}

extern "C" int picoCloseSerial(int Chan)
{
  return 0;
}

extern "C" int picoWriteSerial(int Chan, char * Buffer, int Len)
{
  Chan--;

   if (isUSB[Chan])
    return USBHandles[Chan]->write(Buffer,Len);

  // Need to check available. Looks like it only returns 1/0 not actual space so have to check for each byte 

  for (int n = 0; n < Len; n++)
  {
    while(Handles[Chan]->availableForWrite() == 0)
    {
      BPQFastPoll(); 
    }
    Handles[Chan]->write(Buffer[n]);
  }

//n = Handles[Chan]->write(Buffer,Len);
 // if (n != Len)

  return n;
}

extern "C" int picoReadSerial(int Chan, char * Buffer, int Len)
{
  int ptr = 0;
  int ret;

  Chan--;

  if (isUSB[Chan])
  {
    while (USBHandles[Chan]->available() && ptr < Len)
    {
      ret = USBHandles[Chan]->read();
      if (ret == -1)
        break;
       Buffer[ptr++] = ret;
    }
  }
  else
  {
    int n = Handles[Chan]->available();

   // if (n)
   //   Debugprintf("Aval %d", n);

    if (Handles[Chan]->available() && ptr < Len)
    {
      ret = Handles[Chan]->read();
      Buffer[ptr++] = ret;
    }
  }
  return ptr;
}


extern "C" int DoRoutes();
extern "C" int DoNodes();

extern "C" int SaveNodes ()
{
	cfg = LittleFS.open("BPQNODES.dat", "w");

  if (!cfg)
  {
    CONSOLE->println("BPQNODES create failed");
    return 0;
  }

  DoRoutes();
  DoNodes();
  
  cfg.close();
	return (0);
}

extern "C" void writeNodesLine(char * line)
{
  cfg.write(line, strlen(line));
}

File root;
File file;

extern "C" void OpenDirectory()
{
  root = LittleFS.open("/", "r");
  file = root.openNextFile();
}

extern "C" void GetNextDirEntry(char * line)
{
  if (file)
  {
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




extern "C" int picoDelete(char * from)
{
  return LittleFS.remove(from);
}



extern "C" int picoRename(char * from, char * to)
{
  return LittleFS.rename(from, to);
}


extern "C" File * picoOpenFile(char * FN)
{
  if(!LittleFS.exists(FN))
    return 0;

  File * file = 0;
  file = new(File);

  *file = LittleFS.open(FN, "r");

  return file;
}

extern "C" int picoGetLine(File * file, char * Line, int maxline)
{
  int n = file->readBytesUntil('\n', Line, maxline);
  Line[n] = 0;

  return n;
}

extern "C" void picoCloseFile(File * file)
{
  file->close();
  delete file;
}

extern "C" int GetNextFileBlock(File * file, char * line)
{
  int n = file->read((unsigned char *)line, 128);
  line[n] = 0;

  return n;
}


extern "C" File * picoCreateFile(char * FN)
{
  File * file = 0;
  file = new(File);

  *file = LittleFS.open(FN, "w");

  return file;
}




extern "C" void picofprintf(File * file, const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);
	va_start(arglist, format);
	int n = vsprintf(Mess, format, arglist);

  file->write(Mess, n);
}
  
extern"C" void picoWriteLine(File * file, char * Data, int len)
{
  file->write(Data, len);
}
  
extern "C" int createandwritefile(char * FN, char * Data, int len)
{
  File file = LittleFS.open(FN, "w");

  if (file)
  {
    file.write(Data, len);
    file.close();

    return 1;
  }
  
  return 0;     // open failed
}
