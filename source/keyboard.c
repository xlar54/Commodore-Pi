#include "types.h"
#include "usbd/usbd.h"
#include "device/hid/keyboard.h"
#include "terminal.h"
#include "string.h"
#include "keyboard.h"


char* gKeynames[] =
{
	"A", // 0
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K", // 10
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U", // 20
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"1", 
	"2", 
	"3", 
	"4", 
	"5", // 30 
	"6", 
	"7", 
	"8", 
	"9",
	"0", 
	"RETURN",
	"ESC",
	"BSP",
	"TAB",
	"SPACE", // 40
	"MINUS",
	"EQUALS",
	"LSQUAREBRACKET",
	"RSQUAREBRACKET",
	"BACKSLASH", // NO
	"HASH",
	"SEMICOLON",
	"APOSTROPHE",
	"GRAVE",
	"COMMA", // 50
	"FULLSTOP",
	"FORWARDFLASH",
	"CAPS-LOCK",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7", // 60
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"PRTSCN",
	"SCROLL-LOCK",
	"BREAK",
	"INSERT",
	"HOME", // 70
	"PAGE-UP",
	"DELETE",
	"END",
	"PAGE-DOWN",
	"RIGHT",  //5
	"LEFT", // 6
	"DOWN", // 7
	"UP", // 8
	"NUM-LOCK",
	"NUMDIV", // 80
	"NUMMUL",
	"NUMSUB",
	"NUMADD",
	"NUMRETURN",
	"NUM1",
	"NUM2",
	"NUM3",
	"NUM4",
	"NUM5",
	"NUM6", // 90
	"NUM7",
	"NUM8",
	"NUM9",
	"NUM10",
	"NUMDELETE",
	"BACKSLASH",
	"RMENU"
}; 

const char gVirtualTable_shifted[] =
{
	0, 
	0, 
	0, 
	0, 
	'A', 
	'B', 
	'C', 
	'D', 
	'E', 
	'F', 
	'G',
	'H', 
	'I', 
	'J',
	'K',  // 10 
	'L', 
	'M',
	'N',	
	'O', 
	'P',
	'Q', 
	'R', 
	'S', 
	'T', 
	'U',  // 20
	'V', 
	'W', 
	'X', 
	'Y', 
	'Z',
	'!',  
	'"', 
	'*', 
	'$', // 30
	'%', 
	'^', 
	'&', 
	'*',
	'(',
	')',
	'\r', 
	/*esc*/0, 
	/*Bsp*/0, 
	'\t',
	' ', // 40
	'_', 
	'+', 
	'{',
	'}',
	'|',
	'~',  
	':', 
	'@', 
	'*', 
	'<', // 50
	'>', 
	'?', 
	0, 
	0,
	0,
	0, 
	0, 
	0, 
	0, 
	0, // 60
	0, 
	0, 
	0, 
	0, 
	0,
	0, 
	0, 
	0, 
	0, 
	0, // 70
	0, 
	0, 
	0, 
	0, 
	0,
	0,  
	0, 
	0, 
	0, 
	'/', // 80
	'*', 
	'-', 
	'+', 
	'\n',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6', // 90
	'7',
	'8',
	'9',
	'0',
	0, // Numpad delete
	'\\'
};

const char gVirtualTable[] = 
{
	0, 
	0, 
	0, 
	0, 
	'A', 
	'B', 
	'C', 
	'D', 
	'E', 
	'F', 
	'G',
	'H', 
	'I', 
	'J',
	'K',  // 10 
	'L', 
	'M',
	'N',	
	'O', 
	'P',
	'Q', 
	'R', 
	'S', 
	'T', 
	'U',  // 20
	'V', 
	'W', 
	'X', 
	'Y', 
	'Z',
	'1',  
	'2', 
	'3', 
	'4', // 30
	'5', 
	'6', 
	'7', 
	'8',
	'9',
	'0',
	'\r', 
	/*esc*/27, 
	/*Bsp*/20, 
	'\t',
	' ', // 40
	'-', 
	'=', 
	'[',
	']',
	'\\',
	'#',  
	';', 
	'\'', 
	'`', 
	',', // 50
	'.', 
	'/', 
	0, 
	0,
	0,
	0, 
	0, 
	0, 
	0, 
	0, // 60
	0, 
	0, 
	0, 
	0, 
	0,
	0, 
	0, 
	0, 
	0, 
	0, // 70
	0, 
	0, 
	0, 
	0, 
	29,
	157,  
	17, 
	145, 
	0, 
	'/', // 80
	'*', 
	'-', 
	'+', 
	'\n',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6', // 90
	'7',
	'8',
	'9',
	'0',
	20, // Numpad delete
	'\\'
};

bool gHaveKeyboard;
unsigned int gKeyboardAddr;
unsigned short gLastKeystate[6];
struct KeyboardModifiers gLastModifiers;

bool KeyboardCtrlDown(void)
{
	return gLastModifiers.LeftControl || gLastModifiers.RightControl;
}

bool KeyboardShiftDown(void)
{
	return gLastModifiers.LeftShift || gLastModifiers.RightShift;
}

virtualkey ScanToVirtual(unsigned int scanCode)
{
	if(scanCode < 0) return -1;
	
	return (virtualkey)(scanCode - 4);
}

unsigned char VirtualToAsci(virtualkey vk, bool shifted)
{
	if(vk < 0 || vk > (sizeof(gVirtualTable) / sizeof(gVirtualTable[0]))) return -1;
	
	if(shifted)
		return gVirtualTable_shifted[vk + 4];
	else
		return gVirtualTable[vk + 4];
}

char* GetKeyName(char* buf, unsigned int bufLen, virtualkey vk)
{
	char* keyName = gKeynames[vk];
	
	if(strlen(keyName) < bufLen)
		strcpy(keyName, buf);

	return buf;
}

// Retrieves the address of the first attached keyboard
bool EnsureKeyboard(void)
{	
	// KeyboardUpdate() modifies this
	if(gHaveKeyboard) 
		return true;
	
	UsbCheckForChange();
	
	if(KeyboardCount() == 0)
	{
		gHaveKeyboard = false;
		return false;
	}
	
	gKeyboardAddr = KeyboardGetAddress(0);
	gHaveKeyboard = true;
	
	return true;
}

bool KeyWasDown(unsigned short scanCode)
{
	unsigned int i;
	for(i = 0; i < 6; i++)
		if(gLastKeystate[i] == scanCode) return true;
		
	return false;
}

// Gets the scanCode of the first currently pressed key (0 if none)
unsigned short KeyboardGetChar(void)
{
	unsigned int i;
	for(i = 0; i < 6; i++)
	{
		unsigned short key = KeyboardGetKeyDown(gKeyboardAddr, i);
		
		if(key == 0) return 0;
		if(KeyWasDown(key)) continue;
		if(key >= 104) continue;
		
		return key;
	}
	
	return 0;
}

unsigned int KeyboardInitialise(void)
{
	gHaveKeyboard = false;
	gKeyboardAddr = 0;

	unsigned int i;
	for(i = 0; i < 6; i++)
		gLastKeystate[i] = 0;
	
	return 0;
}

void KeyboardUpdate(void)
{
	if(!EnsureKeyboard())
	{
		print("Failed to update keyboard, could not obtain device.", strlen("Failed to update keyboard, could not obtain device."));
		return;
	}
	
	unsigned int i;
	for(i = 0; i < 6; i++)
		gLastKeystate[i] = KeyboardGetKeyDown(gKeyboardAddr, i);
		
	gHaveKeyboard = KeyboardPoll(gKeyboardAddr) == 0;
	gLastModifiers = KeyboardGetModifiers(gKeyboardAddr);
}
