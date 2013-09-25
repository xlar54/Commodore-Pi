#ifndef PIOS_KEYBOARD_C
#define PIOS_KEYBOARD_C
#include "types.h"

typedef enum 
{
	VK_A, 
	VK_B, 
	VK_C, 
	VK_D, 
	VK_E, 
	VK_F, 
	VK_G, 
	VK_H, 
	VK_I, 
	VK_J,
	VK_K, // 10
	VK_L, 
	VK_M, 
	VK_N, 
	VK_O, 
	VK_P, 
	VK_Q, 
	VK_R, 
	VK_S, 
	VK_T,
	VK_U, // 20
	VK_V, 
	VK_W, 
	VK_X, 
	VK_Y, 
	VK_Z, 
	VK_0, 
	VK_1, 
	VK_2, 
	VK_3,
	VK_4, // 30
	VK_5, 
	VK_6, 
	VK_7, 
	VK_8, 
	VK_9, 
	VK_ENTER, 
	VK_ESC, 
	VK_BSP, 
	VK_TAB,
	VK_SPACE, // 40 
	VK_MINUS, 
	VK_EQUALS, 
	VK_SBLEFT, 
	VK_SBRIGHT, 
	VK_BACKSLASH, 
	VK_HASH, 
	VK_SEMICOLON, 
	VL_APOSTROPHE, 
	VK_GRAVE, 
	VK_COMMA, // 50
	VK_FULLSTOP, 
	VK_FORWARDSLASH, 
	VK_CAPS, 
	VK_F1, 
	VK_F2, 
	VK_F3, 
	VK_F4, 
	VK_F5, 
	VK_F6, 
	VK_F7, // 60
	VK_F8, 
	VK_F9, 
	VK_F10, 
	VK_F11, 
	VK_F12, 
	VK_PRTSCN, 
	VK_SCL, 
	VK_BREAK, 
	VK_INSERT, 
	VK_HOME, // 70
	VK_PGUP,  // 71
	VK_DELETE,  // 72
	VK_END,  //73
	VK_PG_DN,  // 74
	VK_RIGHT,  // 75
	VK_LEFT,  // 76
	VK_DOWN,  // 77
	VK_UP,  // 78
	VK_NUMLOCK, // 79
	VK_NUM_DIV, // 80
	VK_NUM_MUL, 
	VK_NUM_SUB, 
	VK_NUM_ADD, 
	VK_NUM_ENTER,  // 83
	VK_NUM1, 
	VK_NUM2,  // 85
	VK_NUM3, 
	VK_NUM4,  // 87
	VK_NUM5, 
	VK_NUM6,  // 90
	VK_NUM7, 
	VK_NUM8, 
	VK_NUM9, 
	VK_NUM0, 
	VK_NUM_DEL,
	VK_BACKSLASH2,
	VK_RMENU 
} virtualkey;

unsigned int KeyboardInitialise(void);
void KeyboardUpdate(void);
unsigned short KeyboardGetChar(void);
bool EnsureKeyboard(void);
virtualkey ScanToVirtual(unsigned int scanCode);
unsigned char VirtualToAsci(virtualkey vk, bool shiftDown);
char* GetKeyName(char* buf, unsigned int bufLen, virtualkey vk);
bool KeyboardShiftDown(void);
bool KeyboardCtrlDown(void);

#endif
