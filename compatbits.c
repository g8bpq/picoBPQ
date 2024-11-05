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

/*

Stuff to make compiling on WINDOWS and LINUX easier

*/




#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>

#define BOOL int

#define VOID void
#define UCHAR unsigned char
#define USHORT unsigned short
#define ULONG uint32_t
#define UINT unsigned int
#define SHORT short
#define DWORD int32_t

#define APIENTRY

#define TRUE 1
#define FALSE 0
#define FAR

#define HWND unsigned int
#define HINSTANCE unsigned int

#define strtok_s strtok_r

VOID Debugprintf(const char * format, ...);

int memicmp(unsigned char *a, unsigned char *b, int n)
{
	if (n)
	{
		while (n && (toupper(*a) == toupper(*b)))
			n--, a++, b++;

		if (n)
			return toupper(*a) - toupper(*b);
   }
   return 0;
}
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2)
{
    unsigned char c1, c2;
    int  v;

	if (pStr1 == NULL)
	{
		if (pStr2)
			Debugprintf("stricmp called with NULL 1st param - 2nd %s ", pStr2);
		else
			Debugprintf("stricmp called with two NULL params");

		return 1;
	}


    do {
        c1 = *pStr1++;
        c2 = *pStr2++;
        /* The casts are necessary when pStr1 is shorter & char is signed */
        v = tolower(c1) - tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0') );

    return v;
}
char * strupr(char* s)
{
  char* p = s;

  if (s == 0)
	  return 0;

  while (*p = toupper( *p )) p++;
  return s;
}

char * strlwr(char* s)
{
  char* p = s;
  while (*p = tolower( *p )) p++;
  return s;
}

int sprintf_s(char * string, int plen, const char * format, ...)
{
	va_list(arglist);
	int Len;

	va_start(arglist, format);
	Len = vsprintf(string, format, arglist);
	va_end(arglist);
	return Len;
}


void closesocket(int sock)
{
	if (sock)
		close(sock);
}


