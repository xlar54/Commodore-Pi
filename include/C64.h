#ifndef _CB64W_H
#define _CB64W_H

typedef short 				SHORT;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;

#define TRUE 1
#define FALSE 0
#define NULL 0

// ---------------------------------------------------------------------------
// Cb64 Defines

#define VERMAJ 0
#define VERMIN 9
#define VERMINTWO 8

#define DEFAULTEMULATEDJOYSTICK 1
#define DEFAULTGAMMA 1.0
#define DEFAULTSKIPRATE 1
#define DEFAULTGRAYSCALEON 0
#define DEFAULTILLEGALOPCODE 0

#define ATNOR 1
#define CLKOR 2
#define INITOR 4
#define DATAOR 8
#define ATNAND 254
#define CLKAND 253
#define INITAND 251
#define DATAAND 247

typedef union
{
	struct { BYTE l,h; } B;
	WORD W;
} CB64_pair;

typedef struct 
{
	BYTE C64CART[16];
	DWORD headerlen;
	WORD version;
	WORD hardtype;
	BYTE exromline;
	BYTE gameline;
	BYTE dummy[6];
	BYTE name[20];
	BYTE chip[4];
	DWORD packlen;
	WORD chiptype;
	WORD bank;
	WORD address;
	WORD length;
} CARTRIDGE;

// ---------------------------------------------------------------------------
// Globals

extern BOOL g_fPAL;

extern BYTE *BasicPointer;
extern BYTE *KernalPointer;
extern BYTE *RAMPointer;
extern BYTE *CharPointer;
extern BYTE *ColorPointer;
extern BYTE *Virtualscreen; 

extern int done,reset,percent,skipper;
extern int cycles,cyc_orig,cyc_last;
extern char illopc;
extern char NMI,IRQ;

extern BYTE SIDRegs[0x1D];

extern int Cartflg;
extern CARTRIDGE cart;

// ---------------------------------------------------------------------------
// Functions
extern void CB64_ClearMem();
extern void CB64_MainLoop();
extern void CB64_Draw();
extern BYTE Peek(WORD address);
extern WORD Deek(WORD address);
extern void Poke(WORD address, BYTE value);

#endif