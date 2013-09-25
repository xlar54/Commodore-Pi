//#define WINDOWS32 1

#ifdef WINDOWS32
#include <windows.h>
#include <vfw.h>
#else
#include "c64types.h"
#endif

#include "resource.h"
#include "c64proc.h"
#include "cb64w.h"
#include "tables.h"

// ---------------------------------------------------------------------------
// Globals

#ifdef WINDOWS32
HINSTANCE g_hInst;
HWND g_hWndMain;
HMENU g_hMenu;

HDRAWDIB hdd;
#endif

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

WORD vic_ecm,vic_bmm,vic_mcm,vic_den,vic_x,vic_y,vic_rsel,vic_csel,vic_xscrl,
vic_yscrl,vic_vm,vic_cb,vic_rasirq;
char vic_ec,vic_b0c,vic_b1c,vic_b2c,vic_b3c;
WORD vic_bank;

WORD latch1a=0, latch2a=0,latch1b=0,latch2b=0;

char NMI=0,IRQ=0;
BYTE ICR1set=0, enable1a=0, enable1b=0,ICR2set=0, enable2a=0, enable2b=0,sidon=1;

int percent=0;
int skipper=DEFAULTSKIPRATE;
char illopc=DEFAULTILLEGALOPCODE;

int cycles=0;
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

int cmain(void)
{
}


void CB64_KBReset(void)
{
	memset( key_pressed, 255, 256 );
	memset( key, 0, 256 );
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
	BYTE ret;
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
		case 1: cpuport=ret;
			ret=RAMPointer[0];
			//no break on purpose
		case 0:	RAMPointer[0]=ret;
			RAMPointer[1]&=~(ret|0x20);
			RAMPointer[1]|=(cpuport&ret)|(0x17&~ret);
			break;
		default:RAMPointer[address]=ret;
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
				case 0x11:      vic_ecm=ret&0x40;
						vic_bmm=ret&0x20;
						vic_den=ret&0x10;
						vic_rsel=ret&8;
						vic_yscrl=ret&7;
						vic_rasirq=VICRegs[0x12];
						if(ret&0x80)
							vic_rasirq|=0x100;
						break;
				case 0x12:	vic_rasirq=ret;
						if(VICRegs[0x11]&0x80)
							vic_rasirq|=0x100;
						break;
				case 0x16:	ret|=0xA0;
						vic_mcm=ret&0x10;
						vic_csel=ret&8;
						vic_xscrl=ret&7;
						break;
				case 0x18:      ret|=1;
						vic_vm=((short)ret&0xF0)<<6;
						vic_cb=((short)ret&0xE)<<10;
						break;
				case 0x19:      ret=~(ret&0xF)&VICRegs[0x19];
						if(!(ret&VICRegs[0x1A]&0xF)&&(ret&0x80))
						{
							IRQ=0;
							ret&=0x7f;
						}
						ret|=0x70;
						break;
				case 0x1A:	ret|=0xF0;
                                		break;
				case 0x20: 	vic_ec=ret&0xF;
						ret|=0xF0;
						break;
				case 0x21:	vic_b0c=ret&0xf;
						ret|=0xF0;
						break;
				case 0x22:	vic_b1c=ret&0xf;
						ret|=0xF0;
						break;
				case 0x23:	vic_b2c=ret&0xf;
						ret|=0xF0;
						break;
				case 0x24:	vic_b3c=ret&0xf;
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
				ColorPointer[address&0x3ff]=ret&0xF;
			else if(address<0xDC00); //Nothing, I think
			else if(address<0xDD00)	//CIA 1
			{
				address&=0xF;
				CIA1Regs[address]=ret;
				switch(address)
				{
				case 4:	
					latch1a&=0xFF00;
					latch1a|=value;
					break;
				case 5: 
					latch1a&=0xFF;
					latch1a|=(WORD)value<<8;
					break;
				case 6:	
					latch1b&=0xFF00;
					latch1b|=value;
					break;
				case 7: 
					latch1b&=0xFF;
					latch1b|=(WORD)value<<8;
				case 13:
					if(value&0x01)	
						if(value&0x80)	
							ICR1set|=1;
						else 		
							ICR1set&=254;
					if(value&0x02) 	
						if(value&0x80) 	
							ICR1set|=2;
						else 		
							ICR1set&=253;
					if(value&0x04) 	
						if(value&0x80) 	
							ICR1set|=4;
						else 		
							ICR1set&=251;
					if(value&0x08) 	
						if(value&0x80) 	
							ICR1set|=8;
						else 		
							ICR1set&=247;
					if(value&0x10) 	
						if(value&0x80) 	
							ICR1set|=16;
						else 		
							ICR1set&=239;
					break;
				case 14:
					if(value&0x10)
					{
						ret&=0xEF;
						t1a = latch1a;
						CIA1Regs[4] = latch1a;
					}
					enable1a=ret&0x01;
					break;
				case 15:
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
				case 0:	vic_bank=(~value&3)<<14;
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
				case 4:	latch2a&=0xFF00;
					latch2a|=value;
					break;
				case 5: latch2a&=0xFF;
					latch2a|=(WORD)value<<8;
					break;
				case 6:	latch2b&=0xFF00;
					latch2b|=value;
					break;
				case 7: latch2b&=0xFF;
					latch2b|=(WORD)value<<8;
				case 13:if(value&0x01) 	if(value&0x80) 	ICR2set|=1;
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
				case 14:if(value&0x10)
					{
						ret&=0xEF;
						t2a = latch2a;
						CIA2Regs[4] = latch2a;
					}
					enable2a=ret&1;
					break;
				case 15:if(value&0x10)
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
	g_nFramesGenerated++;
}

// ---------------------------------------------------------------------------
// VIC emulation

void CB64_VICDrawLine()
{
	int scrn_y = vic_y - 50;
	WORD line;
	BYTE *scrn,*chr,*clrp,*result;
	
	static BYTE data[327];

	bcolor[vic_y]=vic_ec;

	if((scrn_y<0)||(scrn_y>199))
		;
	else if(!vic_den||(!vic_rsel&&((scrn_y<4)||(scrn_y>=196))))
		memset(Virtualscreen+(scrn_y<<8)+(scrn_y<<6),vic_ec,320);
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
			WORD line2 = (( scrn_y-vic_yscrl+3 ) >> 3 ) * ( 40 * 8 );
			int temp2=vic_bank+vic_cb+line2+((scrn_y-vic_yscrl+3)&7);
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
			int temp2=vic_bank+vic_cb+((scrn_y-vic_yscrl+3)&7);
			if((temp2&0x7000)==0x1000)
				chr=&CharPointer[temp2&0xFFF];
			else	
				chr=&RAMPointer[temp2];
		}
		clrp=&ColorPointer[line];
		result=&data[vic_xscrl];

		memset( data, vic_ec, 320 );

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

					for ( int i = 0; i < 40; i++ )
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

					for ( int i = 0; i < 40; i++ )
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

					for ( int i = 0; i < 40; i++ )
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

					for ( int i = 0; i < 40; i++ )	// screen is 40 chars wide.
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

					for ( int i = 0; i < 40; i++ )	// screen is 40 chars wide.
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

		BYTE *l_pScr = Virtualscreen + scrn_y * 320;

		if(vic_csel)
		{
			memcpy( l_pScr, data, 320 );
		}
		else
		{
			memset( l_pScr, vic_ec, 7 );
			l_pScr += 7;
			memcpy( l_pScr, data + 7, 320 - 14 );
			l_pScr += 320 - 14;
			memset( l_pScr, vic_ec, 7 );
		}
	}
}

void VICDoLine(void)
{
	CB64_VICDrawLine();

	vic_x=0;
	if( ++vic_y == ( g_fPAL ? 312 : 262 ) )
	{
		//CB64_Sprite();
		CB64_Flip();
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

// ---------------------------------------------------------------------------

void CB64_Draw( DWORD * bits )
{
	BYTE * l_pScrn = Virtualscreen;

	if ( g_fShowBorder )
	{
		// With border: 360x240 display.

		DWORD * bits2 = bits;

		// draw screen

		bits += 360*219 + 20;

		for ( int y = 199; y >= 0; y-- )
		{
			for ( int x = 0; x < 320; x++ )
				*bits++ = cbmcolor[ *l_pScrn++ ];
 
			bits-= 360 + 320;
		}

		// draw border

		BYTE * l_pColor = & bcolor[239 + 30];

		for (int y = 0; y < 20; y++ )
		{
			DWORD l_dwColor = cbmcolor[ *l_pColor-- ];
			for ( int x = 0; x < 360; x++ )
				*bits2++ = l_dwColor;
		}

		for (int y = 0; y < 200; y++ )
		{
			DWORD l_dwColor = cbmcolor[ *l_pColor-- ];
			for ( int x = 0; x < 20; x++ )
				*bits2++ = l_dwColor;
			bits2 += 320;
			for (int x = 0; x < 20; x++ )
				*bits2++ = l_dwColor;
		}

		for (int y = 0; y < 20; y++ )
		{
			DWORD l_dwColor = cbmcolor[ *l_pColor-- ];
			for ( int x = 0; x < 360; x++ )
				*bits2++ = l_dwColor;
		}
	}
	else
	{
		// No border: standard 320x200 display.

		bits += 320*199;
		for ( int y = 199; y >= 0; y-- )
		{
			for ( int x = 0; x < 320; x++ )
				*bits++ = cbmcolor[ *l_pScrn++ ];
 
			bits-= 320*2;
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

void CB64_EmulateCycle()
{
	// VIC 

	vic_x+=8;
	if ( vic_x >= ( g_fPAL ? 506 : 512 ) )
	{
		VICDoLine();
	}
	
	// 6510

	CB64_6510();

	CIATimers();
	
	cycles++;
}


BOOL CB64_AllocMem(void)
{
	//RAMPointer = new BYTE[65536];
	//ColorPointer = new BYTE[1024];
	//Virtualscreen = new BYTE[65536];

	RAMPointer = RAMPointerBuffer;
	ColorPointer = ColorPointerBuffer;
	Virtualscreen = VirtualscreenBuffer;

	return 1;
	/*(
		RAMPointer != NULL &&
		ColorPointer != NULL &&
		Virtualscreen != NULL
	);*/
}
	
void CB64_ClearMem()
{
	for ( int i = 0; i < 65536; i++ )
	{
		if ( i & 0x80 )
			RAMPointer[i] = 0x66;
		else
			RAMPointer[i] = 0x99;
	}

	memset( ColorPointer, 0, 1024 );
	memset( VICRegs, 0, 0x2F );
	memset( SIDRegs, 0, 0x1D );
	memset( CIA1Regs, 0, 0x10 );
	memset( CIA2Regs, 0, 0x10 );

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
}

BOOL CB64_Init()
{
	if ( !CB64_AllocMem() )
		return FALSE;

	CB64_KBReset();
	CB64_ClearMem();

	return TRUE;
}

void CB64_Term()
{

}


#ifdef WINDOWS32
// ---------------------------------------------------------------------------
// Windows-specific functions

// ---------------------------------------------------------------------------
// HexWndProc

int hexDY = 0;
LRESULT CALLBACK HexWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// WndProc

DWORD * bits = NULL;
BITMAPINFO bmpinfo;
HBITMAP hbmp = NULL;

void CreateDisplay()
{
	g_xRes = ( g_fShowBorder ) ? 360 : 320;
	g_yRes = ( g_fShowBorder ) ? 240 : 200;

	HDC hdc = GetDC( g_hWndMain );

	bmpinfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmpinfo.bmiHeader.biWidth = g_xRes;
	bmpinfo.bmiHeader.biHeight = g_yRes;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 32;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biXPelsPerMeter = 0;
	bmpinfo.bmiHeader.biYPelsPerMeter = 0;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;
	
	hbmp = CreateDIBSection( hdc, &bmpinfo, DIB_RGB_COLORS, (void **) &bits, NULL, 0 );

	ReleaseDC( g_hWndMain, hdc );

	SetWindowPos(g_hWndMain, NULL, 0, 0, 
		g_xRes + 6, 
		g_yRes+ 44,
		SWP_NOZORDER | SWP_NOMOVE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	HDC hdc;
	PAINTSTRUCT ps;
	
	char filename[ MAX_PATH ];

	filename[0] = '\0';

	switch (uMsg) {

		case WM_CREATE:
			return 0;
		case WM_DESTROY:
			g_fPaused = TRUE;
			PostQuitMessage(0);
			return 0;
		case WM_PAINT:
			hdc = BeginPaint( g_hWndMain, &ps);
			if ( bits )
				DrawDibDraw( hdd, hdc, 0, 0, -1, -1, &bmpinfo.bmiHeader, bits, 0, 0, g_xRes, g_yRes, 0 );
			EndPaint( g_hWndMain, &ps);
			return 0;
		case WM_KEYUP:
			if ( key[wParam] )
			{
				keysdown--;
				key[wParam] = 0;
			}

			key_pressed[key_table[wParam][0]]|=key_table[wParam][1];
			return 0;
		case WM_KEYDOWN:
			if ( !key[wParam] )
			{
				keysdown++;
				key[wParam] = 1;
			}
			RAMPointer[0x277 + RAMPointer[0xc6]++] = wParam; //this is where we can fix some key mapping issues
			return 0;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}



// ---------------------------------------------------------------------------
// Timer callback.

LARGE_INTEGER QPFreq;
LARGE_INTEGER QPLast;


// ---------------------------------------------------------------------------
// WinMain

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wc, wc2;

	/* Initialize the emulator */

	if ( !CB64_Init() )
	{
		MessageBox( NULL, "Emulator initialization failed!", "Damn!", MB_OK);
		return 1;
	}

	/* Register the window class for the main window. */

	if (!hPrevInstance) 
	{
		wc.style = 0;
		wc.lpfnWndProc = (WNDPROC) WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance; 
		wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject( HOLLOW_BRUSH ); //(HBRUSH) (COLOR_BTNFACE+1);
		wc.lpszMenuName =  NULL;
		wc.lpszClassName = "CB64WndClass";
		if (!RegisterClass(&wc))
			return FALSE;

		wc2.style = 0;
		wc2.lpfnWndProc = (WNDPROC) HexWndProc;
		wc2.cbClsExtra = 0;
		wc2.cbWndExtra = 0;
		wc2.hInstance = hInstance; 
		wc2.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
		wc2.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
		wc2.hbrBackground = (HBRUSH) (COLOR_BTNFACE+1);
		wc2.lpszMenuName =  NULL;
		wc2.lpszClassName = "HexWndClass";
		if (!RegisterClass(&wc2))
			return FALSE;
	}
 
	g_hInst = hInstance;  /* save instance handle */

	/* Create the main window. */

	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));

	g_hWndMain = CreateWindow("CB64WndClass", "CB64 | win32",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
		CW_USEDEFAULT, CW_USEDEFAULT,
		320, 200, 
		(HWND) NULL, g_hMenu, g_hInst, (LPVOID) NULL
	);

	if ( !g_hWndMain ) 
		return FALSE;

	ShowWindow(g_hWndMain, nCmdShow);
	UpdateWindow(g_hWndMain);

	hdd = DrawDibOpen();

	QueryPerformanceFrequency( &QPFreq );
	QueryPerformanceCounter( &QPLast ); 


	while ( TRUE )
	{
		while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )  
		{
			if ( msg.message == WM_QUIT ) 
				goto endProg;

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}  

		if ( !bits )
			CreateDisplay();

		if ( g_fPaused )
			WaitMessage();
		else
		{
			if ( g_fTrace )
			{
				WORD l_wOldLoc = curr_loc;

				do
				{
					CB64_EmulateCycle();
				} while ( l_wOldLoc == curr_loc );

				g_fPaused = TRUE;
			}
			else
			{
				LARGE_INTEGER QPTime;
				QueryPerformanceCounter( &QPTime );

				LONG QPNFrames = (LONG) ( ( QPTime.QuadPart - QPLast.QuadPart ) / ( QPFreq.QuadPart / 50 ) );

				QPNFrames = min( QPNFrames, 5 );

				if ( QPNFrames > 0 )
				{
					while ( g_nFramesGenerated < QPNFrames )
						CB64_EmulateCycle();
					g_nFramesGenerated = 0;

					QPLast = QPTime;
				}
				else
				{
					Sleep( 2 );
				}
			}

			CB64_Draw( bits );

			InvalidateRgn( g_hWndMain, NULL, FALSE );
		}
	}

endProg:
	timeKillEvent( g_nTimerID );

	DeleteObject( hbmp );
	DrawDibClose( hdd );
	CB64_Term();

	return msg.wParam;
}

#endif
