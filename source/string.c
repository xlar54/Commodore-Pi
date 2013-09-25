#include "string.h"
#include "terminal.h"

int strlen(char* str)
{
	int length = 0;
	while(*str++)
		length++;

	return length;
}

char* strcpy(const char* src, char* dst)
{
	char *ptr;
	ptr = dst;
	while((*dst++ = *src++));

	return ptr;
}

// TODO: Rewrite this to take in the pointer to a buffer where the result will be stored
void itoa(int number, char* buf)
{
	// We populate the string backwards, increment to make room for \0
	buf++;

	int negative = number < 0;
	if(negative)
	{
		buf++;
		number = -number;
	}

	// Find where our string will end
	int shifter = number;
	do
	{
		buf++;
		shifter /= 10;
	}while(shifter > 0);

	// Make sure the string is terminated nicely
	*--buf = '\0';
	
	// Start converting the digits into characters
	do
	{
		*--buf = '0' + (number % 10); // Muahaha!
		number /= 10;
	}while(number > 0);

	if(negative)
		*--buf = '-';
		
	// Done!
}

void printf(char* text, ...)
{
	// set up variable argument list
	va_list ap;
	va_start(ap, text);

	char res[256];
	char* result = res;
	
	// scan all characters in 'text' and look for format specifiers
	do
	{
		if(*text == '%')
		{
			if(*(text + 1) == 'c') // unsigned char
			{
				*result++ = (char)va_arg(ap, unsigned int);
			}
			else if(*(text + 1) == 'd') // integer (signed)
			{
				char itoBuf[10];
				itoa(va_arg(ap, int), &itoBuf[0]);
				
				char* intstr = strcpy(itoBuf, result);
				
				result += strlen(intstr);
			}
			else if(*(text + 1) == 's') // string
			{
				char* arg = (char*)va_arg(ap, int);
				
				strcpy(arg, result);
				
				result += strlen(arg);
			}
			
			// make sure we skip the type specifier
			text++;
			
			continue;
		}
		
		// if we got thus far, it's probably just a normal character
		*result++ = *text;
	}while(*text++ != '\0');
	
	print(res, strlen(res));
}

void *ucmemset(unsigned char *s, int c, size_t n)
{
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

void *imemset(int *s, int c, size_t n)
{
    int* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}
void *cmemset(char *s, int c, size_t n)
{
    char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(BYTE *dest, const BYTE *src, size_t n)
{
    BYTE *dp = dest;
    const BYTE *sp = src;
    while (n--)
        *dp++ = *sp++;
    return dest;
}