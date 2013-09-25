/*  String.h */
#ifndef STRING_H
#define STRING_H

#include <stdarg.h> // Needed for varying argument length

typedef unsigned char       BYTE;
typedef unsigned int   		size_t;

int strlen(char* str);
char* strcpy(const char* src, char* dst);
void itoa(int number, char* buf);
void printf(char* text, ...);
void *ucmemset(unsigned char *s, int c, size_t n);
void *imemset(int *s, int c, size_t n);
void *cmemset(char *s, int c, size_t n);
void *memcpy(BYTE *dest, const BYTE *src, size_t n);

#endif // STRINGUTIL_H


