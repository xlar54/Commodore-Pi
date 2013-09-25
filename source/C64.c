#include "C64.h"
#include "6510.h"
#include "C64Data.h"
#include "keyboard.h"
#include "string.h"

// http://sta.c64.org/cbm64mem.html

// ---------------------------------------------------------------------------
// Globals
int g_nTimerID = 0;
int g_nFramesGenerated = 0;

BOOL g_fPaused = FALSE;
BOOL g_fTrace = FALSE;

BOOL g_fShowBorder = TRUE;
BOOL g_fPAL = TRUE;

BOOL g_fAudio = FALSE; // Audio services initialized ?

int g_xRes, g_yRes;

// ---------------------------------------------------------------------------
// Cb64 Globals

int key_pressed[256];

BYTE *BasicPointer = ORIGINAL64B;
BYTE *KernalPointer = ORIGINAL64K;
BYTE *RAMPointer;
BYTE *CharPointer = ORIGINAL64C;
BYTE *ColorPointer;
BYTE *Virtualscreen; 

BYTE RAMPointerBuffer[65536];
BYTE ColorPointerBuffer[1024];
BYTE VirtualscreenBuffer[65536];

BYTE joytoemu = DEFAULTEMULATEDJOYSTICK;

BYTE VICRegs[0x2F],SIDRegs[0x1D],CIA1Regs[0x10],CIA2Regs[0x10];

WORD vic_ecm,vic_bmm,vic_mcm,vic_den,vic_x,vic_y,vic_rsel,vic_csel,vic_xscrl,vic_yscrl,vic_vm,vic_cb,vic_rasirq;
char vic_ec,vic_b0c,vic_b1c,vic_b2c,vic_b3c;
WORD vic_bank;

WORD latch1a=0, latch2a=0,latch1b=0,latch2b=0;

char NMI=0,IRQ=0;
BYTE ICR1set=0, enable1a=0, enable1b=0,ICR2set=0, enable2a=0, enable2b=0,sidon=1;

//int percent=0;
//int skipper=DEFAULTSKIPRATE;
//char illopc=DEFAULTILLEGALOPCODE;

int refresh = 0;
int cyc_orig,cyc_last=0;
int PPORTACT=0;

int charrom=1;
int emuid=1;

WORD t1a,t2a,t1b,t2b;

int done,reset;

BYTE bcolor[312];

char key[256], keysdown = 0;

BYTE cpuport;

//char CurrentCrt[MAX_PATH];

int Cartflg = 0;
CARTRIDGE cart;

// ---------------------------------------------------------------------------
// Cb64

void CB64_KBReset(void)
{
	imemset( key_pressed, 255, 256 );
	cmemset( key, 0, 256 );
	keysdown = 0;
}


WORD Deek(WORD address)
{
	return (WORD)Peek(address)+((WORD)Peek(address+1)<<8);
}

BYTE Peek(WORD address)
{
	BYTE ret;
	static BYTE emudetect=0xAA;

	if(address<0xA000)
		ret=RAMPointer[address];
	else if(address<0xC000)
	{
		if((RAMPointer[1]&3)==3)
			ret=BasicPointer[address&0x1FFF];
		else 	ret=RAMPointer[address];
	}
	else if(address<0xD000)
		ret=RAMPointer[address];
	else if(address<0xE000)
	{
		ret=RAMPointer[1];
		if(ret&3)
			if(ret&4)
			{
				//IO chips
				if(address<0xD400) //VIC
				{
					address&=0x3f;
					if(address<0x2f)
						ret=VICRegs[address];
					else 	ret=0xFF;
					switch(address)
					{
					case 0x11:	if(vic_y&0x100)
								ret|=0x80;
							else	ret&=0x7F;
							break;
					case 0x12:	ret=vic_y&0xFF;
							break;
					case 0x19:	//if(key[KB_f12])
								ret|=6;
							break;
					}
				}
				else if(address<0xD800) //SID
				{
					address&=0x1F;
					if(address<0x1d)
						ret=SIDRegs[address];
					else	ret=0xFF;
					if(address==0x1C)
						//ret=SNDvoices[2].envx>>23;
						ret = 0;
				}
				else if(address<0xDBE8) //ColorRAM
					ret=ColorPointer[address&0x3ff]&0xF;
				else if(address<0xDC00) //Nothing, I think
					ret=0xFF;
				else if(address<0xDD00) //CIA 1
				{
					address&=0xF;
					ret=CIA1Regs[address];
					switch(address)
					{
					case 0:	if(joytoemu==1)
							ret=(ret&224);//+ joyemu();
						break;
					case 1:	ret=key_pressed[255-CIA1Regs[0]];
						if(joytoemu==2&&ret==255)
							ret=(ret&224);//+joyemu();
						break;
					case 13:IRQ=CIA1Regs[13]=0;
						break;
					}
				}
				else if(address<0xDE00) //CIA 2
				{
					address&=0xF;
					ret=CIA2Regs[address];
					switch(address)
					{
					case 0:	/*ret=inp(PPORT);*/ ret=0;
						if(ret&CLKOR)
							CIA2Regs[0]&=0xBF;
						else	CIA2Regs[0]|=0x40;
						if(ret&DATAOR)
							{
							 CIA2Regs[0]&=0x7F;
							 PPORTACT=1;
							}
						else	CIA2Regs[0]|=0x80;
						ret=CIA2Regs[0];
						break;
					case 13:CIA2Regs[13]=IRQ=0;
						break;
					}
				}
				else if(address<0xDFA0) //Nothing or CIA2 echoes, not sure
					ret=0xFF;
				else
				{ret=0;
				      if (emuid==1) {
					switch(address)
					{
					 case 0xdfa0: ret='C'; break;
					 case 0xdfa1: ret='O'; break;
					 case 0xdfa2: ret='M'; break;
					 case 0xdfa3: ret='E'; break;
					 case 0xdfa4: ret=' '; break;
					 case 0xdfa5: ret='B'; break;
					 case 0xdfa6: ret='A'; break;
					 case 0xdfa7: ret='C'; break;
					 case 0xdfa8: ret='K'; break;
					 case 0xdfa9: ret=' '; break;
					 case 0xdfaa: ret='6'; break;
					 case 0xdfab: ret='4'; break;
					 case 0xdfac: ret=','; break;
					 case 0xdfad: ret=' '; break;
					 case 0xdfae: ret='V'; break;
					 case 0xdfaf: ret=VERMAJ+'0'; break;
					 case 0xdfb0: ret='.'; break;
					 case 0xdfb1: ret=VERMIN+'0'; break;
					 case 0xdfb2: ret=VERMINTWO+'0'; break;
					 case 0xdfb3: ret=0x0d; break;
					 case 0xdfb4: ret='C'; break;
					 case 0xdfb5: ret='O'; break;
					 case 0xdfb6: ret='P'; break;
					 case 0xdfb7: ret='Y'; break;
					 case 0xdfb8: ret='R'; break;
					 case 0xdfb9: ret='I'; break;
					 case 0xdfba: ret='G'; break;
					 case 0xdfbb: ret='H'; break;
					 case 0xdfbc: ret='T'; break;
					 case 0xdfbd: ret=' '; break;
					 case 0xdfbe: ret='1'; break;
					 case 0xdfbf: ret='9'; break;
					 case 0xdfc0: ret='9'; break;
					 case 0xdfc1: ret='8'; break;
					 case 0xdfc2: ret=' '; break;
					 case 0xdfc3: ret='B'; break;
					 case 0xdfc4: ret='Y'; break;
					 case 0xdfc5: ret=' '; break;
					 case 0xdfc6: ret='J'; break;
					 case 0xdfc7: ret='.'; break;
					 case 0xdfc8: ret='F'; break;
					 case 0xdfc9: ret='I'; break;
					 case 0xdfca: ret='T'; break;
					 case 0xdfcb: ret='I'; break;
					 case 0xdfcc: ret='E'; break;
					 case 0xdfcd: ret=' '; break;
					 case 0xdfce: ret='A'; break;
					 case 0xdfcf: ret='N'; break;
					 case 0xdfd0: ret='D'; break;
					 case 0xdfd1: ret=' '; break;
					 case 0xdfd2: ret='B'; break;
					 case 0xdfd3: ret='.'; break;
					 case 0xdfd4: ret='M'; break;
					 case 0xdfd5: ret='A'; break;
					 case 0xdfd6: ret='R'; break;
					 case 0xdfd7: ret='T'; break;
					 case 0xdfd8: ret='I'; break;
					 case 0xdfd9: ret='N'; break;
					 case 0xdfda: ret=0x00; break;
					 case 0xdffc: ret=VERMIN*0x10+VERMINTWO; break;
					 case 0xdffd: ret=VERMAJ; break;
					 case 0xdffe: ret='J'; break;
					 case 0xdfff: {ret=emudetect;emudetect=~emudetect;} break;
					}
				      }
				}
			}
			else	ret=CharPointer[address&0x0FFF];
		else	ret=RAMPointer[address];
	}
	else	if(RAMPointer[1]&2)
			ret=KernalPointer[address&0x1FFF];
		else	ret=RAMPointer[address];
	return(ret);
}

void Poke(WORD address, BYTE value)
{
	register BYTE ret;
	ret=value;

	// ADD BY MAX
	///////////////////////
	// IF ANY CARTRIDGE IN USE WE CANNOT TO PERFORM WRITE OPERATIONS
	// IN CARTRIDGE AREA (I THINK).
	if (Cartflg && address>0x8000 && address<0x9FFF) 
		return;
	////////////////////////////////////////////////////////////

	if(address<0xD000)
	{
		switch(address)
		{
			case 1:   // processor port
				cpuport=ret;
				ret=RAMPointer[0];
				//no break on purpose
			case 0:	  // Processor port data direction register
				RAMPointer[0]=ret;
				RAMPointer[1]&=~(ret|0x20);
				RAMPointer[1]|=(cpuport&ret)|(0x17&~ret);
				break;
			default:  // Memory location 0x0002 - 0xCFFF
				RAMPointer[address]=ret;
		}
	}
	else if(address<0xE000)
	{
		if((RAMPointer[1]&7)>4)	//Write to I/O
		{
			if(address<0xD400)	//VIC
			{
				address&=0x3F;
				switch(address)
				{
					case 0x11: // VIC Control Register 1
						vic_ecm=ret&0x40;         // Bit 6   : Extended background mode on
						vic_bmm=ret&0x20;         // Bit 5   : Bitmap Mode: 0= text,        1 = Bitmap
						vic_den=ret&0x10;         // Bit 4   : Screen Mode: 0= screen off,  1= Screen on
						vic_rsel=ret&8;           // Bit 3   : Rows select: 0= 24 rows,     1=25 rows
						vic_yscrl=ret&7;          // Bits 0-2: Vertical Raster Scroll
						vic_rasirq=VICRegs[0x12]; // Bit 7   : Read- Current raster line (bit 8)   
						if(ret&0x80)       
							vic_rasirq|=0x100;    // Bit 7   : Write- Raster line to generate interrupt (bit 8)
						break;
					case 0x12: // Raster Counter
						vic_rasirq=ret;           // Read - Current raster line (bits 0 - 7)  
						if(VICRegs[0x11]&0x80)    // Write- Raster line to generate interrupt at (bits 0 - 7)  
							vic_rasirq|=0x100;
						break;
					case 0x16: // VIC Control Register 2
						ret|=0xA0;
						vic_mcm=ret&0x10;         // Multicolor mode on
						vic_csel=ret&8;           // Screen width 0 = 38 columns, 1 = 40 columns
						vic_xscrl=ret&7;          // Horizontal raster scroll
						break;
					case 0x18: // Memory Pointers
						ret|=1;
						vic_vm=((short)ret&0xF0)<<6;
						vic_cb=((short)ret&0xE)<<10;
						break;
					case 0x19: // Interrupt Status Register    
						ret=~(ret&0xF)&VICRegs[0x19];
						if(!(ret&VICRegs[0x1A]&0xF)&&(ret&0x80))	// Bit 0 = Acknowledge raster interrupt
						{											// Bit 1 = Acknowledge sprite-background collision interrupt
							IRQ=0;									// Bit 2 = Acknowledge sprite-sprite collision interrupt
							ret&=0x7f;								// Bit 3 = Acknowledge light pen interrupt
						}
						ret|=0x70;
						break;
					case 0x1A: // Interrupt Enabled  
						ret|=0xF0;					// Bit 0 = Raster interrupt enabled
													// Bit 1 = Sprite background collision interrupt enabled
													// Bit 2 = Sprite - Sprite collision interrupt enabled
													// Bit 3 = Light pen interrupt enabled
						break;
					case 0x20: // Border color 	
						vic_ec=ret&0xF;
						ret|=0xF0;
						break;
					case 0x21: // Background color 0
						vic_b0c=ret&0xf;
						ret|=0xF0;
						break;
					case 0x22: // Background color 1
						vic_b1c=ret&0xf;
						ret|=0xF0;
						break;
					case 0x23: // Background color 2
						vic_b2c=ret&0xf;
						ret|=0xF0;
						break;
					case 0x24: // Background color 3
						vic_b3c=ret&0xf;
						ret|=0xF0;
						break;
				}
				if(address<0x2f)
					VICRegs[address]=ret;
			}
			else if(address<0xD800) //SID
			{
				/*address&=0x1F;
				if(address<0x1d)
				{
					ret=SIDRegs[address];
					SIDRegs[address]=value;
				}
				switch(address)
				{
				case 4: if((ret^value)&0xF1)
						if(value&1)
							SNDNoteOn(0);
						else	SNDNoteOff(0);
					break;
				case 5: SNDvoices[0].ar=ret>>4;
					SNDvoices[0].dr=ret&0xF;
					break;
				case 6:	SNDvoices[0].sr=ret&0xf;
					SNDvoices[0].sl=ret>>4;
					break;
				case 11:if((ret^value)&0xF1)
						if(value&1)
							SNDNoteOn(1);
						else	SNDNoteOff(1);
					break;
				case 12:SNDvoices[1].ar=ret>>4;
					SNDvoices[1].dr=ret&0xF;
					break;
				case 13:SNDvoices[1].sr=ret&0xf;
					SNDvoices[1].sl=ret>>4;
					break;
				case 18:if((ret^value)&0xF1)
						if(value&1)
							SNDNoteOn(2);
						else	SNDNoteOff(2);
					break;
				case 19:SNDvoices[2].ar=ret>>4;
					SNDvoices[2].dr=ret&0xF;
					break;
				case 20:SNDvoices[2].sr=ret&0xf;
					SNDvoices[2].sl=ret>>4;
					break;
				}*/
			}
			else if(address<0xDBE8) //ColorRAM
				ColorPointer[address&0x3ff]=ret&0xF;  // Only bits 0-3
			else if(address<0xDC00); //Unused
			else if(address<0xDD00)	//CIA 1
			{
				address&=0xF;
				CIA1Regs[address]=ret;
				switch(address)
				{
					case 4:	// Timer A:  Set timer start value
						latch1a&=0xFF00;
						latch1a|=value;
						break;
					case 5: // Timer A:  Set timer start value
						latch1a&=0xFF;
						latch1a|=(WORD)value<<8;
						break;
					case 6:	// Timer B:  Set timer start value
						latch1b&=0xFF00;
						latch1b|=value;
						break;
					case 7: // Timer B:  Set timer start value
						latch1b&=0xFF;
						latch1b|=(WORD)value<<8;
						break;
					case 8: // Set TOD or alarm time
						break;
					case 9: // Set TOD or alarm time
						break;
					case 10: // Set TOD or alarm time
						break;
					case 11: // Set TOF or alarm time
						break;
					case 12: // Serial shift register
						break;
					case 13: // Interrupt control and status register
						if(value&0x01)			// Bit 0: Enable interrupts generated by timer A underflow
							if(value&0x80)	
								ICR1set|=1;
							else 		
								ICR1set&=254;
						if(value&0x02) 			// Bit 1: Enable interrupts generated by timer B underflow
							if(value&0x80) 	
								ICR1set|=2;
							else 		
								ICR1set&=253;
						if(value&0x04) 			// Bit 2: Enable TOD alarm interrupt
							if(value&0x80) 	
								ICR1set|=4;
							else 		
								ICR1set&=251;
						if(value&0x08) 			// Bit 3: Enable interrupts generated by a byte having been recieved/sent via serial shift register
							if(value&0x80) 	
								ICR1set|=8;
							else 		
								ICR1set&=247;
						if(value&0x10) 			// Bit 4: Enable interrupts generated by positive edge on FLAG pin
							if(value&0x80) 	
								ICR1set|=16;
							else 		
								ICR1set&=239;
						break;
					case 14: // Timer A control register
						if(value&0x10)
						{
							ret&=0xEF;
							t1a = latch1a;
							CIA1Regs[4] = latch1a;
						}
						enable1a=ret&0x01;
						break;
					case 15: //Timer B control register
						if(value&0x10)
						{
							ret&=0xEF;
							t1b = latch1b;
							CIA1Regs[6] = latch1b;
						}
						enable1b=ret&0x1;
						break;
				}
			}
			else if(address<0xDE00)
			{
				address&=0xF;
				CIA2Regs[address]=ret;
				switch(address)
				{
					case 0:	 // Port A serial bus access
						vic_bank=(~value&3)<<14;
						ret=244;
						if(value&0x08)
							ret|=ATNOR;
						if(value&0x10)
							ret|=CLKOR;
						if(value&0x20)
						{
						 ret|=DATAOR;
						 PPORTACT=2;
						}
						/*outp(PPORT,ret);*/
						break;
					case 1: // Port B rs232 access
						break;
					case 2: // Port A data direction register
						break;
					case 3: // Port B data direction register
						break;
					case 4:	// Timer A - Set timer start value
						latch2a&=0xFF00;
						latch2a|=value;
						break;
					case 5: // Timer A - Set timer start value
						latch2a&=0xFF;
						latch2a|=(WORD)value<<8;
						break;
					case 6:	// Timer B - Set timer start value
						latch2b&=0xFF00;
						latch2b|=value;
						break;
					case 7: // Timer B - Set timer start value
						latch2b&=0xFF;
						latch2b|=(WORD)value<<8;
					case 13: // Interrupt control and status register
						if(value&0x01) 	if(value&0x80) 	ICR2set|=1;
								else 		ICR2set&=254;
						if(value&0x02) 	if(value&0x80) 	ICR2set|=2;
								else 		ICR2set&=253;
						if(value&0x04) 	if(value&0x80) 	ICR2set|=4;
								else 		ICR2set&=251;
						if(value&0x08) 	if(value&0x80) 	ICR2set|=8;
								else 		ICR2set&=247;
						if(value&0x10) 	if(value&0x80) 	ICR2set|=16;
								else 		ICR2set&=239;
						break;
					case 14: // Timer A control register
						if(value&0x10)
						{
							ret&=0xEF;
							t2a = latch2a;
							CIA2Regs[4] = latch2a;
						}
						enable2a=ret&1;
						break;
					case 15: // Timer B control register
						if(value&0x10)
						{
							ret&=0xEF;
							t2b = latch2b;
							CIA2Regs[6] = latch2b;
						}
						enable2b=ret&1;
						break;
				}
			}
		}
		else RAMPointer[address]=ret;
	}
	else RAMPointer[address]=ret;
}

void Singlecolsprite (BYTE primecol,WORD spriteloc,BYTE dubx,BYTE duby,BYTE addx,int xpos,int ypos)
{
	BYTE x1,x2,y,check,curr;
	WORD screenpos; int xnow,ynow;
	xpos+=addx*256;
	for (y=0;y<21;y++) 
	{
		for (x1=0;x1<3;x1++) 
		{
			curr=RAMPointer[spriteloc+x1+3*y]; 
			check=128;
			for (x2=0;x2<8;x2++) 
			{
				if (curr>=check) 
				{
					curr=curr-check;
					screenpos=((x1*8+x2)*(dubx+1)+xpos)+320*(y*(duby+1)+ypos);
					xnow=(x1*8+x2)*(dubx+1)+xpos;
					ynow=(y*(duby+1)+ypos);
					if (xnow>=0&&xnow<=319&&ynow>=0&&ypos+y<=199) 
						Virtualscreen[screenpos]=primecol;
					if (dubx==1&&xnow+1>=0&&xnow+1<=319&&ynow>=0&&ynow<=199) 
						Virtualscreen[screenpos+1]=primecol;
					if (duby==1&&xnow>=0&&xnow<=319&&ynow+1>=0&&ynow+1<=199) 
						Virtualscreen[screenpos+320]=primecol;
					if (dubx==1&&duby==1&&xnow+1>=0&&xnow+1<=319&&ynow+1>=0&&ynow+1<=199) 
						Virtualscreen[screenpos+321]=primecol;
				}
				check=check / 2;
			} 
		} 
	}
}

void Multicolsprite (BYTE primecol,BYTE coltwo,BYTE colthree,WORD spriteloc,BYTE dubx,BYTE duby,BYTE addx,int xpos,int ypos)
{
	BYTE x1,x2,y,check,curr,col,multicol,colors[4];
	WORD screenpos; int xnow,ynow;
	xpos+=addx*256;
	colors[1]=coltwo;//VICRegs[0x25];
	colors[3]=colthree;//VICRegs[0x26];
	colors[2]=primecol;
	col=colors[2];
	for (y=0;y<21;y++) 
	{
		for (x1=0;x1<3;x1++) 
		{
			curr=RAMPointer[spriteloc+x1+3*y]; check=128;
			for (x2=0;x2<4;x2++) 
			{
				multicol=0;
				if (curr>=check) 
				{
					curr=curr-check; 
					multicol+=2;
				} 
				check=check/2;
				if (curr>=check) 
				{
					curr=curr-check; 
					multicol+=1;
				} 
				check=check/2;
				col=colors[multicol];
				if (multicol!=0) 
				{
					screenpos=(x1*8+x2*2)*(dubx+1)+xpos+320*((y)*(duby+1)+ypos);
					xnow=(x1*8+x2*2)*(dubx+1)+xpos;
					ynow=((y)*(duby+1)+ypos);
					if (xnow>=0&&xnow<=319&&ynow>=0&&ypos+y<=199) 
						Virtualscreen[screenpos]=col;
					if (xnow+1>=0&&xnow+1<=319&&ynow>=0&&ypos+y<=199) 
						Virtualscreen[screenpos+1]=col;
					if (dubx==1) 
					{
						if (xnow+2>=0&&xnow+2<=319&&ynow>=0&&ypos+y<=199) 
							Virtualscreen[screenpos+2]=col;
						if (xnow+3>=0&&xnow+3<=319&&ynow>=0&&ypos+y<=199) 
							Virtualscreen[screenpos+3]=col;
					}
					if (duby==1) 
					{
						if (xnow>=0&&xnow<=319&&ynow+1>=0&&ypos+y+1<=199) 
							Virtualscreen[screenpos+320]=col;
						if (xnow+1>=0&&xnow+1<=319&&ynow+1>=0&&ypos+y+1<=199) 
							Virtualscreen[screenpos+321]=col;
					}
					if (dubx==1&&duby==1) 
					{
						if (xnow+2>=0&&xnow+2<=319&&ynow+1>=0&&ypos+y+1<=199) 
							Virtualscreen[screenpos+322]=col;
						if (xnow+3>=0&&xnow+3<=319&&ynow+1>=0&&ypos+y+1<=199) 
							Virtualscreen[screenpos+323]=col;
					}
				} 
			} 
		} 
	}
}

/*void CB64_Sprite()
{
	int powtwo = 128;

	for ( int x=0; x<8; x++ )
	{
		if ( ( VICRegs[0x15 ] & powtwo ) / powtwo == 1 ) 
		{
			int hibyte = ( VICRegs[0x10] & powtwo ) / powtwo;
			int xpos = VICRegs[0+(7-x)*2] - 24;
			int ypos = VICRegs[1+(7-x)*2] - 50;
			if ( ( VICRegs[0x1C ] & powtwo ) / powtwo == 1) 
			{
				Multicolsprite(VICRegs[0x27+(7-x)],VICRegs[0x25],VICRegs[0x26],(RAMPointer[vic_vm+vic_bank+0x3f8+(7-x)])*64+vic_bank,(VICRegs[0x1D]&powtwo)/powtwo,(VICRegs[0x17]&powtwo)/powtwo,hibyte,xpos,ypos);
			}
			else 
			{
				Singlecolsprite(VICRegs[0x27+(7-x)],(RAMPointer[vic_vm+vic_bank+0x3f8+(7-x)])*64+vic_bank,(VICRegs[0x1D]&powtwo)/powtwo,(VICRegs[0x17]&powtwo)/powtwo,hibyte,xpos,ypos);
			} 
		}
		powtwo = powtwo / 2;
	}

}*/

void CB64_Flip()
{
}

// ---------------------------------------------------------------------------
// VIC emulation

void CB64_VICDrawLine()
{
	int scrn_y = vic_y - 50;
	WORD line;
	WORD line2;
	BYTE *scrn,*chr,*clrp,*result;
	int temp2;
	int i;
	BYTE *l_pScr;
	int x,y;
	
	static BYTE data[327];

	bcolor[vic_y]=vic_ec;

	if((scrn_y<0)||(scrn_y>199))
		;
	else if(!vic_den||(!vic_rsel&&((scrn_y<4)||(scrn_y>=196))))
		ucmemset(Virtualscreen+(scrn_y<<8)+(scrn_y<<6),vic_ec,320);
	else
	{
		line=(scrn_y-vic_yscrl+3)&0xFFF8;
		line+=line<<2;
				
		if ( vic_bmm ) 
		{
			int temp1 =vic_vm+line+vic_bank;
			if((temp1 &0x7000)==0x1000)
				scrn=&CharPointer[temp1&0xFFF];
			else	
				scrn=&RAMPointer[temp1];
			line2 = (( scrn_y-vic_yscrl+3 ) >> 3 ) * ( 40 * 8 );
			temp2=vic_bank+vic_cb+line2+((scrn_y-vic_yscrl+3)&7);
			if((temp2&0x7000)==0x1000)
				chr=&CharPointer[temp2&0xFFF];
			else	
				chr=&RAMPointer[temp2];
		}
		else
		{
			int temp1 =vic_vm+line+vic_bank;
			if((temp1 &0x7000)==0x1000)
				scrn=&CharPointer[temp1&0xFFF];
			else	
				scrn=&RAMPointer[temp1];
			temp2=vic_bank+vic_cb+((scrn_y-vic_yscrl+3)&7);
			if((temp2&0x7000)==0x1000)
				chr=&CharPointer[temp2&0xFFF];
			else	
				chr=&RAMPointer[temp2];
		}
		clrp=&ColorPointer[line];
		result=&data[vic_xscrl];

		ucmemset( data, vic_ec, 320 );

		if ( vic_mcm )
		{
			if ( vic_bmm )
			{
				if ( vic_ecm )
				{
					// ECM/BMM/MCM: Invalid bitmap mode 2

					// black is displayed.
				}
				else
				{

					// ecm/BMM/MCM: Multicolor bitmap mode

					for (i = 0; i < 40; i++ )
					{
						int l_nChar = chr[ i << 3 ];
						BYTE l_bCol10 = *scrn & 0xf;
						BYTE l_bCol01 = (*scrn++ & 0xf0) >> 4;
						BYTE l_bCol11 = *clrp++;

						if ( l_nChar & 128 )
							*result = ( l_nChar & 64 ) ? l_bCol11 : l_bCol10;
						else
							*result = ( l_nChar & 64 ) ? l_bCol01 : vic_b0c;
						*(result+1) = *result;
						result += 2;

						if ( l_nChar & 32 )
							*result = ( l_nChar & 16 ) ? l_bCol11 : l_bCol10;
						else
							*result = ( l_nChar & 16 ) ? l_bCol01 : vic_b0c;
						*(result+1) = *result;
						result += 2;

						if ( l_nChar & 8 )
							*result = ( l_nChar & 4 ) ? l_bCol11 : l_bCol10;
						else
							*result = ( l_nChar & 4 ) ? l_bCol01 : vic_b0c;
						*(result+1) = *result;
						result += 2;
						
						if ( l_nChar & 2 )
							*result = ( l_nChar & 1 ) ? l_bCol11 : l_bCol10;
						else
							*result = ( l_nChar & 1 ) ? l_bCol01 : vic_b0c;
						*(result+1) = *result;
						result += 2;
					}
				}
			}
			else
			{
				if ( vic_ecm )
				{
					// ECM/bmm/MCM: Invalid text mode

					// black is displayed.
				}
				else
				{
					// ecm/bmm/MCM: Multicolor text mode

					for (i = 0; i < 40; i++ )
					{
						int l_nChar = chr[ *scrn++ << 3 ];
						BYTE l_bCol = *clrp++;

						if ( l_bCol & 0x08 )
						{
							l_bCol &= 0x07;

							if ( l_nChar & 128 )
								*result = ( l_nChar & 64 ) ? l_bCol : vic_b2c;
							else
								*result = ( l_nChar & 64 ) ? vic_b1c : vic_b0c;
							*(result+1) = *result;
							result += 2;

							if ( l_nChar & 32 )
								*result = ( l_nChar & 16 ) ? l_bCol : vic_b2c;
							else
								*result = ( l_nChar & 16 ) ? vic_b1c : vic_b0c;
							*(result+1) = *result;
							result += 2;

							if ( l_nChar & 8 )
								*result = ( l_nChar & 4 ) ? l_bCol : vic_b2c;
							else
								*result = ( l_nChar & 4 ) ? vic_b1c : vic_b0c;
							*(result+1) = *result;
							result += 2;
							
							if ( l_nChar & 2 )
								*result = ( l_nChar & 1 ) ? l_bCol : vic_b2c;
							else
								*result = ( l_nChar & 1 ) ? vic_b1c : vic_b0c;
							*(result+1) = *result;
							result += 2;
						}
						else
						{
							*result++ = ( l_nChar & 128 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 64 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 32 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 16 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 8 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 4 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 2 ) ? l_bCol : vic_b0c;
							*result++ = ( l_nChar & 1 ) ? l_bCol : vic_b0c;
						}
					}
				}
			}
		}
		else
		{
			if ( vic_bmm )
			{
				if ( vic_ecm )
				{
					// ECM/BMM/mcm: Invalid bitmap mode 1

					// black is displayed.
				}
				else
				{
					// ecm/BMM/mcm: Standard bitmap mode

					for (i = 0; i < 40; i++ )
					{
						int l_nChar = chr[ i << 3 ];
						BYTE l_bCol0 = *scrn & 0xf;
						BYTE l_bCol1 = (*scrn++ & 0xf0) >> 4;

						*result++ = ( l_nChar & 128 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 64 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 32 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 16 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 8 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 4 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 2 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 1 ) ? l_bCol1 : l_bCol0;
					}
				}

			}
			else			
			{
				if ( vic_ecm )
				{
					// ECM/bmm/mcm: ECM text mode

					for (i = 0; i < 40; i++ )	// screen is 40 chars wide.
					{
						BYTE l_bCol0 = *scrn >> 6;
						BYTE l_bCol1 = *clrp++;
						int l_nChar = chr[ ( *scrn++ & 0x3f ) << 3 ];
						
						*result++ = ( l_nChar & 128 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 64 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 32 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 16 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 8 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 4 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 2 ) ? l_bCol1 : l_bCol0;
						*result++ = ( l_nChar & 1 ) ? l_bCol1 : l_bCol0;
					}
				}
				else
				{
					// ecm/bmm/mcm: Standard text mode

					for (i = 0; i < 40; i++ )	// screen is 40 chars wide.
					{
						int l_nChar = chr[ *scrn++ << 3 ];
						BYTE l_bCol = *clrp++;
						
						*result++ = ( l_nChar & 128 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 64 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 32 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 16 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 8 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 4 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 2 ) ? l_bCol : vic_b0c;
						*result++ = ( l_nChar & 1 ) ? l_bCol : vic_b0c;
					}
				}
			}
		}

		l_pScr = Virtualscreen + scrn_y * 320;
vic_csel=1;
		if(vic_csel)
		{
			memcpy( l_pScr, data, 320 );
				
			//for(x=0;x<320;x++)
			//	DrawPixel(x+20,scrn_y+20, cbmcolor[data[x]]);	
		}
		else
		{
			ucmemset( l_pScr, vic_ec, 7 );
			l_pScr += 7;
			memcpy( l_pScr, data + 7, 320 - 14 );
			l_pScr += 320 - 14;
			ucmemset( l_pScr, vic_ec, 7 );
		}
	}
}

// ---------------------------------------------------------------------------

void CB64_Draw()
{
	BYTE * l_pScrn = Virtualscreen;
	BYTE * l_pColor;
	int y ,x;

	if ( g_fShowBorder )
	{
		// With border: 360x240 display.
		// draw screen
		for (y = 0; y < 199; y++ )
		{
			for (x = 0; x < 320; x++ )
				DrawPixel(x+20,y+20, cbmcolor[*l_pScrn++]); 
		}

		// draw border
		//l_pColor = & bcolor[239 + 30];
		DrawFilledRectangle(0,0,359,19, cbmcolor[vic_ec]);     //top
		DrawFilledRectangle(0,220,359,239, cbmcolor[vic_ec]);  //bottom  
		DrawFilledRectangle(0,19,19,219, cbmcolor[vic_ec]);    // left
		DrawFilledRectangle(340,19,359,219, cbmcolor[vic_ec]); //right
	}
	else
	{
		// No border: standard 320x200 display.
		for (y = 0; y < 199; y++ )
		{
			for (x = 0; x < 320; x++ )
				DrawPixel(x+20,y+20, cbmcolor[*l_pScrn++]); 
		}	
	}
}

void CIATimers(void)
{
	if(enable1a)
	{
		if(t1a==0)
		{
			t1a=*(SHORT *)&CIA1Regs[4]=latch1a;
			if(CIA1Regs[0xE]&0x08) CIA1Regs[0xE]&=254;
			if(ICR1set&0x01)
			{
				IRQ=1;
				CIA1Regs[0xD]=0x80;
			}
			CIA1Regs[0xD]|=1;
			if((CIA1Regs[0xF]&0x60)==0x40)
				CIA1Regs[6] = --t1b;
		}
		CIA1Regs[4]=--t1a;
	}
	if(enable1b)
	{
		if(t1b==0)
		{
			t1b=*(SHORT *)&CIA1Regs[6]=latch1b;
			if(CIA1Regs[0xF]&0x08) CIA1Regs[0xF]&=254;
			if(ICR1set&0x02)
			{
				IRQ=1;
				CIA1Regs[0xD]=0x80;
			}
			CIA1Regs[0xD]|=2;
		}
		else
		{
			if((CIA1Regs[0xF]&0x60)==0)
			{
				CIA1Regs[6]=--t1b;
			}
		}
	}
	// end CIA 1
	// CIA 2
	if(enable2a)
	{
		if(t2a==0)
		{
			t2a=*(SHORT *)&CIA2Regs[4]=latch2a;
			if(CIA2Regs[0xE]&0x08) CIA2Regs[0xE]&=254;
			if(ICR2set&0x01)
			{
				NMI=1;
				CIA2Regs[0xD]=0x80;
			}
			CIA2Regs[0xD]|=1;
			if((CIA2Regs[0xF]&0x60)==0x40)
				CIA2Regs[0x6]=--t2b;
		}
		CIA2Regs[0x4]=--t2a;
	}
	if(enable2b)
	{
		if(t2b==0)
		{
			t2b=*(SHORT *)&CIA2Regs[6]=latch2b;
			if(CIA2Regs[0xF]&0x08) CIA2Regs[0xF]&=254;
			if(ICR2set&0x02)
			{
				NMI=1;
				CIA2Regs[0xD]=0x80;
			}
			CIA2Regs[0xD]|=2;
		}
		else if((CIA2Regs[0xF]&0x60)==0) CIA2Regs[0x6]=--t2b;
	}
	// end CIA 2
}

void CB64_Keyboard()
{
	// This slows us down considerably...

	KeyboardUpdate();
	
	short scanCode = KeyboardGetChar();		
		
	// Nothing pressed
	if(scanCode == 0)
		return;

	virtualkey vk = ScanToVirtual(scanCode);
		
	char c = VirtualToAsci(vk, KeyboardShiftDown());
	/*
	char name[15];
	char* keyname = GetKeyName(name, 15, vk);
	CB64_Print("                                       ", 0,0);
	CB64_Print(keyname, 0,0);		*/
	
	
	if(c == 27)
		RAMPointer[145] = 127;
	else
		// Equiv to Poke 631,x : Poke 198,1
		RAMPointer[0x277 + RAMPointer[0xc6]++] = c; //this is where we can fix some key mapping issues
}

inline void CB64_EmulateCycle()
{
	// VIC 

	vic_x+=8;
	if ( vic_x >= ( g_fPAL ? 506 : 512 ) )
	{
         if (refresh == 2)
          CB64_VICDrawLine();
		vic_x=0;
			
		if( ++vic_y == ( g_fPAL ? 312 : 262 ) )
		{
			//CB64_Sprite();
			//CB64_Flip();
			refresh++;
			vic_y = 0;
		}

		if(vic_y==vic_rasirq)
		{
			VICRegs[0x19]|=1;
			if(VICRegs[0x1A]&0x1)
			{
				IRQ=1;
				VICRegs[0x19]|=0x80;
			}
		}
	}
	
	// 6510
	CB64_6510();
	CIATimers();
}

void CB64_ClearMem()
{
	int i;

	for ( i = 0; i < 65536; i++ )
	{
		if ( i & 0x80 )
			RAMPointer[i] = 0x66;
		else
			RAMPointer[i] = 0x99;
	}

	ucmemset( ColorPointer, 0, 1024 );
	ucmemset( VICRegs, 0, 0x2F );
	ucmemset( SIDRegs, 0, 0x1D );
	ucmemset( CIA1Regs, 0, 0x10 );
	ucmemset( CIA2Regs, 0, 0x10 );

	RAMPointer[0]=0x2F;
	RAMPointer[1]=0x37;
	enable1a = 0;
	enable1b = 0;
	enable2a = 0;
	enable2b = 0;
	vic_den = 0;
	vic_ec = 0;
	vic_b0c = 0;
	done = reset = vic_x = vic_y = 0;
	vic_rasirq = 0xFFFF;
	
	Poke(53280, 3);
}

void CB64_DrawBorders()
{
	BYTE bcolor;
	bcolor = Peek(53280) % 15;
	DrawFilledRectangle(0,0,359,239, cbmcolor[bcolor]); 
	/*DrawFilledRectangle(0,0,359,19, cbmcolor[bcolor]);     //top
	DrawFilledRectangle(0,200,359,239, cbmcolor[bcolor]);  //bottom  
	DrawFilledRectangle(0,19,19,219, cbmcolor[bcolor]);    // left
	DrawFilledRectangle(340,19,359,219, cbmcolor[bcolor]); //right*/
}

void CB64_Print(char *s, int row, int col)
{
	unsigned int i = 0;
	unsigned int c = 0;
	int screen = 1024;
	
	while(s[i] !=0)
	{
		c = s[i++];
		
		if(c >= 0 && c < 32) 
			c += 128;
		else if (c > 63 && c < 96)
			c -= 64;
		else if (c > 95 && c < 128)
			c -=32;
		else if (c > 127 && c < 160)
			c += 64;
		else if (c > 159 && c < 192)
			c -= 64;
		else if (c > 191 && c < 224)
			c -= 128;
		else if (c > 223 && c < 255)
			c -= 128;
		else if (c == 255)
			c = 94;
		
		RAMPointer[screen * (row+1) + col] =c;
		col++;
		
		if(col > 39)
		{
			col=0; row++;
			if(row > 24)
				row = 24;
		}
	};

}

void CB64_MainLoop()
{
	refresh = 0;

	RAMPointer = RAMPointerBuffer;
	ColorPointer = ColorPointerBuffer;
	Virtualscreen = VirtualscreenBuffer;
	
	CB64_KBReset();
	CB64_ClearMem();
	CB64_DrawBorders();
	
	while(1)
	{
		CB64_EmulateCycle();
		
		if(refresh == 3)
		{
			CB64_Keyboard();
			CB64_Draw();
			refresh = 0;
		}
	}
}




