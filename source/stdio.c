#include "types.h"
#include "usbd/usbd.h" // For UsbCheckForChange()
#include "device/hid/keyboard.h"
#include "terminal.h"
#include "string.h"
#include "keyboard.h"

void cin(char* buf, unsigned int bufLen)
{
	unsigned int readChars = 0;
	char temp[bufLen];
	
	if(!EnsureKeyboard()) 
		return; // We have no keyboard
	
	do
	{
		char k = KeyboardGetChar();
		if(k == 0)
			continue;
		
		if(k == '\n')
			break;

		temp[readChars] = k;
		readChars++;

	}while(readChars < bufLen); // Just keep reading (Return cancels this early)

	if(readChars < bufLen - 1)
		temp[readChars + 1] = '\0';
	else
		temp[bufLen - 1] = '\0';

	// Copy the read stuff into the buffer
	strcpy(temp, buf);
}