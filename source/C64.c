#include "C64.h"
#include "6510.h"
#include "C64Data.h"
#include "keyboard.h"
#include "string.h"

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


inline WORD Deek(WORD address)
{
	return (WORD)Peek(address)+((WORD)Peek(address+1)<<8);
}

inline BYTE Peek(WORD address)
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
					ret=0;
			}
			else	ret=CharPointer[address&0x0FFF];
		else	ret=RAMPointer[address];
	}
	else	if(RAMPointer[1]&2)
			ret=KernalPointer[address&0x1FFF];
		else	ret=RAMPointer[address];
	return(ret);
}

inline void Poke(WORD address, BYTE value)
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
			case 1: 
				cpuport=ret;
				ret=RAMPointer[0];
				//no break on purpose
			case 0:	
				RAMPointer[0]=ret;
				RAMPointer[1]&=~(ret|0x20);
				RAMPointer[1]|=(cpuport&ret)|(0x17&~ret);
				break;
			default:
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
					case 0x11:      
						vic_ecm=ret&0x40;
						vic_bmm=ret&0x20;
						vic_den=ret&0x10;
						vic_rsel=ret&8;
						vic_yscrl=ret&7;
						vic_rasirq=VICRegs[0x12];
						if(ret&0x80)
							vic_rasirq|=0x100;
						break;
					case 0x12:	
						vic_rasirq=ret;
						if(VICRegs[0x11]&0x80)
							vic_rasirq|=0x100;
						break;
					case 0x16:	
						ret|=0xA0;
						vic_mcm=ret&0x10;
						vic_csel=ret&8;
						vic_xscrl=ret&7;
						break;
					case 0x18:      
						ret|=1;
						vic_vm=((short)ret&0xF0)<<6;
						vic_cb=((short)ret&0xE)<<10;
						break;
					case 0x19:      
						ret=~(ret&0xF)&VICRegs[0x19];
						if(!(ret&VICRegs[0x1A]&0xF)&&(ret&0x80))
						{
							IRQ=0;
							ret&=0x7f;
						}
						ret|=0x70;
						break;
					case 0x1A:	
						ret|=0xF0;
						break;
					case 0x20:	// $D020 53280 - Border color 	
						vic_ec=ret&0xF;
						ret|=0xF0;
						break;
					case 0x21:	// $D021 53281 - Background color 
						vic_b0c=ret&0xf;
						ret|=0xF0;
						break;
					case 0x22:	
						vic_b1c=ret&0xf;
						ret|=0xF0;
						break;
					case 0x23:	
						vic_b2c=ret&0xf;
						ret|=0xF0;
						break;
					case 0x24:	
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
					case 0:	
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
					case 4:	
						latch2a&=0xFF00;
						latch2a|=value;
						break;
					case 5: 
						latch2a&=0xFF;
						latch2a|=(WORD)value<<8;
						break;
					case 6:	
						latch2b&=0xFF00;
						latch2b|=value;
						break;
					case 7: 
						latch2b&=0xFF;
						latch2b|=(WORD)value<<8;
					case 13:
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
	static BYTE pdata[327];

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
		
vic_csel=1;

#ifdef SCANLINE
		if(vic_csel)
		{
			for(x=0;x<320;x++)
				DrawPixel(x+20,scrn_y+20, cbmcolor[data[x]]);
		}
#else		
		l_pScr = Virtualscreen + scrn_y * 320;
		if(vic_csel)
			memcpy( l_pScr, data, 320 );	
#endif
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

inline void CIATimers(void)
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

inline void CB64_Keyboard()
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
	static CB64_pair K;
	static WORD address, jmp_from;
	static BYTE gen1;
	static BYTE curr_inst = 0x80;
	static curr_loc = 0xFCE2;
	static BYTE curr_cycle;
	static BYTE a_reg=0,x_reg=0,y_reg=0,flags=0x30,stack_ptr = 0xFF;

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
	switch(curr_inst) {
	case 0x80:
		if(done)
			return;
		if(reset==1)
		{
			curr_loc= (WORD) (KernalPointer[0x1FFD]<<8) + (WORD) (KernalPointer[0x1FFC]);
			stack_ptr=0xFF;
			flags=0x30;
			CB64_ClearMem();
			IRQ=NMI=reset=0;
		}
		else if(reset==2)
		{
			RAMPointer[1]=55;
			curr_inst=0x80;
			curr_loc=2456;
			IRQ=NMI=reset=0;
		}

		if(IRQ)
		{
			if(flags&FLAG_INT)goto NOIRQ;
			Poke(0x100+stack_ptr--,curr_loc>>8);
			Poke(0x100+stack_ptr--,curr_loc&255);
			Poke(0x100+stack_ptr--,flags&BRK_OFF);
			curr_loc=Deek(0xFFFE);
			flags|=FLAG_INT;
		}
/*on reset sound off		if (curr_loc==0xfce2) {SNDkeys&=~(1<<0);SNDkeys&=~(1<<1);SNDkeys&=~(1<<2);}*/
		/*load program*/
	
		if ( curr_loc==0xf4b8 && Peek(186) == 8 ) 
		{
//			curr_loc=Basic2_ProgLoad();
			if (curr_loc==0xf5d2) 
				flags &= ~FLAG_CARRY;
		}
		

		if(NMI)
		{
			NMI=0;
			Poke(0x100+stack_ptr--,(curr_loc)>>8);
			Poke(0x100+stack_ptr--,(curr_loc)&255);
			Poke(0x100+stack_ptr--,flags);
			curr_loc=Deek(0xFFFA);
		}
NOIRQ:                  
		if((curr_inst=Peek(curr_loc))==0x80)
			curr_inst=0x82;
		curr_cycle=1;
		break;
	case 0xA5: //LDA_Zero();
		if(curr_cycle==1)
		{
			Zero();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(curr_cycle++==2)
			curr_inst=0x80;
		break;
	case 0xD0: //BNE();
		if(curr_cycle==1)curr_loc+=2;
		if((flags&FLAG_ZERO)==0)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
				
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else {curr_inst=0x80;}
		break;
	case 0x4C: //JMP_Abs();
		if(curr_cycle==1)
		{
			jmp_from=curr_loc;
			curr_loc++;
			curr_loc=Deek(curr_loc);
		}
		if(curr_cycle++==2)	curr_inst=0x80;
		break;
	case 0xE8: //INX();
		x_reg++;
		setflags(x_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x10: //BPL();
		if(curr_cycle==1)curr_loc+=2;
		if((flags&FLAG_NEGATIVE)==0)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
				
			}
			if(curr_cycle++==3)
				curr_inst=0x80;
		}
		else
			curr_inst=0x80;
		break;
	case 0xC9: //CMP_Imm();
		Imm();
		CMP();
		curr_inst=0x80;
		break;
	case 0xF0: //BEQ();
		if(curr_cycle==1)curr_loc+=2;
		if(flags&FLAG_ZERO)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else {curr_inst=0x80;}
		break;
	case 0x00: //BRK();
		if(curr_cycle==1)
		{
			Poke(0x100+stack_ptr--,(curr_loc+2)>>8);
			Poke(0x100+stack_ptr--,(curr_loc+2)&255);
			Poke(0x100+stack_ptr--,flags);
			flags|=FLAG_INT;
			curr_loc=Deek(0xFFFE);
		}
		if(curr_cycle++==6)	curr_inst=0x80;
		break;
	case 0x01: //ORA_IndX();
		if(curr_cycle==1)
		{
			IndX();
			ORA();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x05: //ORA_Zero();
		if(curr_cycle==1)
		{
			Zero();
			ORA();
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0x06: //ASL_Zero();
		if(curr_cycle==1)
		{
			Zero();
			ASL();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x08: //PHP();
		if(curr_cycle==1)
		{
			Poke(0x100+stack_ptr--,flags);
			curr_loc+=1;
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0x09: //ORA_Imm();
		Imm();
		ORA();
		curr_inst=0x80;
		break;
	case 0x0A: //ASL_Acc();
		if(a_reg&0x80)flags|=FLAG_CARRY;else flags&=(CRRY_OFF);
		a_reg<<=1;
		setflags(a_reg);
		curr_inst=0x80;
		curr_loc++;
		break;
	case 0x0D: //ORA_Abs();
		if(curr_cycle==1)
		{
			Abs();
			ORA();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x0E: //ASL_Abs();
		if(curr_cycle==1)
		{
			Abs();
			ASL();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;

	case 0x11: //ORA_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			ORA();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x15: //ORA_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			ORA();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x16: //ASL_Zero_X();
		if(curr_cycle++==1)
		{
			Zero_X();
			ASL();
		}
		if(curr_cycle==5)curr_inst=0x80;
		break;
	case 0x18: //CLC();
		flags&=(CRRY_OFF);
		curr_inst=0x80;
		curr_loc++;
		break;
	case 0x19: //ORA_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			ORA();
		}
		if(((address-y_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x1D: //ORA_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			ORA();
		}
		if(((address-x_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else 
			if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x1E: //ASL_Abs_X();
		if(curr_cycle++==1)
		{
			Abs_X();
			ASL();
		}
		if(curr_cycle==6)curr_inst=0x80;
		break;
	case 0x20: //JSR();
		curr_loc+=2;
		jmp_from=curr_loc;
		Poke(0x100+stack_ptr--,curr_loc>>8);
		Poke(0x100+stack_ptr--,curr_loc&255);
		curr_loc=Deek(curr_loc-1);
		curr_inst=0x80;
		break;
	case 0x21: //AND_IndX();
		if(curr_cycle==1)
		{
			IndX();
			AND();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x24: //BIT_Zero();
		if(curr_cycle==1)
		{
			Zero();
			BIT();
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0x25: //AND_Zero();
		if(curr_cycle==1)
		{
			Zero();
			AND();
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0x26: //ROL_Zero();
		if(curr_cycle==1)
		{
			Zero();
			ROL();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x28: //PLP();
		if(curr_cycle==1)
		{
			flags=Peek(0x100+(++stack_ptr))|0x30;
		}
		if(curr_cycle++==3)
		{
			curr_inst=0x80;curr_loc++;
		}
		break;
	case 0x29: //AND_Imm();
		Imm();
		AND();
		curr_inst=0x80;
		break;
	case 0x2A: //ROL_Acc();
		gen1=a_reg<<1;
		if(flags&FLAG_CARRY)gen1|=1;
		if(a_reg&0x80)
			flags|=FLAG_CARRY;
		else
			flags&=CRRY_OFF;
		setflags(gen1);
		a_reg=gen1;
		curr_inst=0x80;curr_loc++;
		break;
	case 0x2C: //BIT_Abs();
		if(curr_cycle==1)
		{
			Abs();
			BIT();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x2D: //AND_Abs();
		if(curr_cycle==1)
		{
			Abs();
			AND();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x2E: //ROL_Abs();
		if(curr_cycle==1)
		{
			Abs();
			ROL();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x30: //BMI();
		if(curr_cycle==1)curr_loc+=2;
		if(flags&FLAG_NEGATIVE)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
				
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else{curr_inst=0x80;}
		break;
	case 0x31: //AND_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			AND();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x35: //AND_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			AND();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x36: //ROL_Zero_X();
		if(curr_cycle++==1)
		{
			Zero_X();
			ROL();
		}
		if(curr_cycle==5)curr_inst=0x80;
		break;
	case 0x38: //SEC();
		flags|=FLAG_CARRY;
		curr_inst=0x80;
		curr_loc++;
		break;
	case 0x39: //AND_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			AND();
		}
		if(((address-y_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x3D: //AND_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			AND();
		}
		if(((address-x_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x3E: //ROL_Abs_X();
		if(curr_cycle++==1)
		{
			Abs_X();
			ROL();
		}
		if(curr_cycle==6)curr_inst=0x80;
		break;
	case 0x40: //RTI();
		if(curr_cycle==1)
		{
			flags=Peek(0x100+(++stack_ptr))|0x30;
			K.B.l=Peek(0x100+(++stack_ptr));
			K.B.h=Peek(0x100+(++stack_ptr));
			curr_loc=K.W;
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0x41: //EOR_IndX();
		if(curr_cycle==1)
		{
			IndX();
			EOR();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x45: //EOR_Zero();
		if(curr_cycle==1)
		{
			Zero();
			EOR();
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0x46: //LSR_Zero();
		if(curr_cycle==1)
		{
			Zero();
			LSR();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x48: //PHA();
		if(curr_cycle==1)
		{
			Poke(0x100+stack_ptr--,a_reg);
			curr_loc++;
		}
		if(curr_cycle++==2)
		{
			curr_inst=0x80;
		}
		break;
	case 0x49: //EOR_Imm();
		Imm();
		EOR();
		curr_inst=0x80;
		break;
	case 0x4A: //LSR_Acc();
		if(a_reg&1)flags|=FLAG_CARRY;else flags&=(CRRY_OFF);
		a_reg>>=1;
		setflags(a_reg);
		curr_inst=0x80;curr_loc++;
		break;
	case 0x4D: //EOR_Abs();
		if(curr_cycle==1)
		{
			Abs();
			EOR();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x4E: //LSR_Abs();
		if(curr_cycle==1)
		{
			Abs();
			LSR();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x50: //BVC();
		if(curr_cycle==1)curr_loc+=2;
		if((flags&FLAG_OVERFLOW)==0)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
				
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else {curr_inst=0x80;}
		break;
	case 0x51: //EOR_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			EOR();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x55: //EOR_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			EOR();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x56: //LSR_Zero_X();
		if(curr_cycle++==1)
		{
			Zero_X();
			LSR();
		}
		if(curr_cycle==5)curr_inst=0x80;
		break;
	case 0x58: //CLI();
		flags&=(INT_OFF);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x59: //EOR_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			EOR();
		}
		if(((address-y_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x5D: //EOR_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			EOR();
		}
		if(((address-x_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x5E: //LSR_Abs_X();
		if(curr_cycle++==1)
		{
			Abs_X();
			LSR();
		}
		if(curr_cycle==6)curr_inst=0x80;
		break;
	case 0x60: //RTS();
		if(curr_cycle==1)
		{
			K.B.l=Peek(0x100+(++stack_ptr));
			K.B.h=Peek(0x100+(++stack_ptr));
			curr_loc=K.W+1;
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0x61: //ADC_IndX();
		if(curr_cycle==1)
		{
			IndX();
			ADC();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x65: //ADC_Zero();
		if(curr_cycle==1)
		{
			Zero();
			ADC();
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0x66: //ROR_Zero();
		if(curr_cycle==1)
		{
			Zero();
			ROR();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x68: //PLA();
		if(curr_cycle==1)
		{
			a_reg=Peek(0x100+(++stack_ptr));
			setflags(a_reg);
		}
		if(curr_cycle++==3)
		{
			curr_inst=0x80;curr_loc+=1;
		}
		break;
	case 0x69: //ADC_Imm();
		Imm();
		ADC();
		curr_inst=0x80;
		break;
	case 0x6A: //ROR_Acc();
		gen1=a_reg>>1;
		if(flags&FLAG_CARRY)gen1|=0x80;
		if(a_reg&1)
			flags|=FLAG_CARRY;
		else
			flags&=CRRY_OFF;
		setflags(gen1);
		a_reg=gen1;
		curr_inst=0x80;curr_loc++;
		break;
	case 0x6C: //JMP_Ind();
		if(curr_cycle==1)
		{
			jmp_from=curr_loc;
			address=Deek(curr_loc+1);
			K.W=address;
			curr_loc=Peek(K.W);
			K.B.l++;
			curr_loc+=Peek(K.W)<<8;
		}
		if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0x6D: //ADC_Abs();
		if(curr_cycle==1)
		{
			Abs();
			ADC();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x6E: //ROR_Abs();
		if(curr_cycle==1)
		{
			Abs();
			ROR();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x70: //BVS();
		if(curr_cycle==1)curr_loc+=2;
		if(flags&FLAG_OVERFLOW)
		{
			if(curr_cycle==2)
			{
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else {curr_inst=0x80;}
		break;
	case 0x71: //ADC_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			ADC();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0x75: //ADC_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			ADC();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0x76: //ROR_Zero_X();
		if(curr_cycle++==1)
		{
			Zero_X();
			ROR();
		}
		if(curr_cycle==5)curr_inst=0x80;
		break;
	case 0x78: //SEI();
		flags|=FLAG_INT;
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x79: //ADC_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			ADC();
		}
		if(((address-y_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x7D: //ADC_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			ADC();
		}
		if(((address-x_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0x7E: //ROR_Abs_X();
		if(curr_cycle++==1)
		{
			Abs_X();
			ROR();
		}
		if(curr_cycle==6)curr_inst=0x80;
		break;
	case 0x81: //STA_IndX();
		if(curr_cycle==1)
		{
			IndX();
			Poke(address,a_reg);
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0x84: //STY_Zero();
		if(curr_cycle==1)
		{
			Zero();
			Poke(address,y_reg);
		}
		if(curr_cycle++==2) curr_inst=0x80;
		break;
	case 0x85: //STA_Zero();
		if(curr_cycle==1)
		{
			Zero();
			Poke(address,a_reg);
		}
		if(curr_cycle++==2) curr_inst=0x80;
		break;
	case 0x86: //STX_Zero();
		if(curr_cycle==1)
		{
			Zero();
			Poke(address,x_reg);
		}
		if(curr_cycle++==2) curr_inst=0x80;
		break;
	case 0x88: //DEY();
		y_reg--;
		setflags(y_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x8A: //TXA();
		a_reg=x_reg;
		setflags(a_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x8C: //STY_Abs();
		if(curr_cycle==1)
		{
			Abs();
			Poke(address,y_reg);
		}
		if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0x8D: //STA_Abs();
		if(curr_cycle==1)
		{
			Abs();
			Poke(address,a_reg);
		}
		if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0x8E: //STX_Abs();
		if(curr_cycle==1)
		{
			Abs();
			Poke(address,x_reg);
		}
		if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0x90: //BCC();
		if(curr_cycle==1)curr_loc+=2;
		if((flags&FLAG_CARRY)==0)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
				
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else {curr_inst=0x80;}
		break;
	case 0x91: //STA_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			Poke(address,a_reg);
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0x94: //STY_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			Poke(address,y_reg);
		}
		if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0x95: //STA_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			Poke(address,a_reg);
		}
		if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0x96: //STX_Zero_Y();
		if(curr_cycle==1)
		{
			Zero_Y();
			Poke(address,x_reg);
		}
		if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0x98: //TYA();
		a_reg=y_reg;
		setflags(a_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x99: //STA_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			Poke(address,a_reg);
		}
		if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0x9A: //TXS();
		stack_ptr=x_reg;
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0x9D: //STA_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			Poke(address,a_reg);
		}
		if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xA0: //LDY_Imm();
		Imm();
		y_reg=Peek(address);
		setflags(y_reg);
		curr_inst=0x80;
		break;
	case 0xA1: //LDA_IndX();
		if(curr_cycle==1)
		{
			IndX();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(curr_cycle++==5)
			curr_inst=0x80;
		break;
	case 0xA2: //LDX_Imm();
		Imm();
		x_reg=Peek(address);
		setflags(x_reg);
		curr_inst=0x80;
		break;
	case 0xA4: //LDY_Zero();
		if(curr_cycle==1)
		{
			Zero();
			y_reg=Peek(address);
			setflags(y_reg);
		}
		if(curr_cycle++==2)
			curr_inst=0x80;
		break;

	case 0xA6: //LDX_Zero();
		if(curr_cycle==1)
		{
			Zero();
			x_reg=Peek(address);
			setflags(x_reg);
		}
		if(curr_cycle++==2)
			curr_inst=0x80;
		break;
	case 0xA8: //TAY();
		y_reg=a_reg;
		setflags(y_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0xA9: //LDA_Imm();
		Imm();
		a_reg=Peek(address);
		setflags(a_reg);
		curr_inst=0x80;
		break;
	case 0xAA: //TAX();
		x_reg=a_reg;
		setflags(x_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0xAC: //LDY_Abs();
		if(curr_cycle==1)
		{
			Abs();
			y_reg=Peek(address);
			setflags(y_reg);
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xAD: //LDA_Abs();
		if(curr_cycle==1)
		{
			Abs();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xAE: //LDX_Abs();
		if(curr_cycle==1)
		{
			Abs();
			x_reg=Peek(address);
			setflags(x_reg);
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xB0: //BCS();
		if(curr_cycle==1)curr_loc+=2;
		if(flags&FLAG_CARRY)
		{
			if(curr_cycle==2)
			{
				
				curr_loc+=(signed char)(gen1=Peek(curr_loc-1));
				if((curr_loc/256)==((curr_loc+(signed char)gen1)/256));
				{
					curr_inst=0x80;
				}
				
			}
			if(curr_cycle++==3)curr_inst=0x80;
		}
		else {curr_inst=0x80;}
		break;
	case 0xB1: //LDA_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(((address-y_reg)/256)!=(address/256))
		{
			if(curr_cycle++==5) curr_inst=0x80;
		}
		else if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xB4: //LDY_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			y_reg=Peek(address);
			setflags(y_reg);
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xB5: //LDA_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xB6: //LDX_Zero_Y();
		if(curr_cycle==1)
		{
			Zero_Y();
			x_reg=Peek(address);
			setflags(x_reg);
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xB8: //CLV();
		flags&=(OVER_OFF);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0xB9: //LDA_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(((address-y_reg)/256)!=(address/256))
		{
			if(curr_cycle++==5) curr_inst=0x80;
		}
		else if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xBA: //TSX();
		x_reg=stack_ptr;
		setflags(x_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0xBC: //LDY_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			y_reg=Peek(address);
			setflags(y_reg);
		}
		if(((address-x_reg)/256)!=(address/256))
		{
			if(curr_cycle++==5) curr_inst=0x80;
		}
		else if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xBD: //LDA_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			a_reg=Peek(address);
			setflags(a_reg);
		}
		if(((address-x_reg)/256)!=(address/256))
		{
			if(curr_cycle++==5) curr_inst=0x80;
		}
		else if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xBE: //LDX_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			x_reg=Peek(address);
			setflags(x_reg);
		}
		if(((address-y_reg)/256)!=(address/256))
		{
			if(curr_cycle++==5) curr_inst=0x80;
		}
		else if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xC0: //CPY_Imm();
		Imm();
		CPY();
		curr_inst=0x80;
		break;
	case 0xC1: //CMP_IndX();
		if(curr_cycle==1)
		{
			IndX();
			CMP();
		}
		if(curr_cycle++==5)
			curr_inst=0x80;
		break;
	case 0xC4: //CPY_Zero();
		if(curr_cycle==1)
		{
			Zero();
			CPY();
		}
		if(curr_cycle++==2)
			curr_inst=0x80;
		break;
	case 0xC5: //CMP_Zero();
		if(curr_cycle==1)
		{
			Zero();
			CMP();
		}
		if(curr_cycle++==2)
			curr_inst=0x80;
		break;
	case 0xC6: //DEC_Zero();
		if(curr_cycle==1)
		{
			Zero();
			Poke(address,gen1=Peek(address)-1);
			setflags(gen1);
		}
		if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xC8: //INY();
		y_reg++;
		setflags(y_reg);
		curr_loc++;
		curr_inst=0x80;
		break;

	case 0xCA: //DEX();
		x_reg--;
		setflags(x_reg);
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0xCC: //CPY_Abs();
		if(curr_cycle==1)
		{
			Abs();
			CPY();
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xCD: //CMP_Abs();
		if(curr_cycle==1)
		{
			Abs();
			CMP();
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xCE: //DEC_Abs();
		if(curr_cycle==1)
		{
			Abs();
			Poke(address,gen1=Peek(address)-1);
			setflags(gen1);
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;

	case 0xD1: //CMP_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			CMP();
		}
		if(((address-y_reg)/256)!=(address/256))
		{
			if(curr_cycle++==5) curr_inst=0x80;
		}
		else if(curr_cycle++==4) curr_inst=0x80;
		break;
	case 0xD5: //CMP_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			CMP();
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xD6: //DEC_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			Poke(address,gen1=Peek(address)-1);
			setflags(gen1);
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0xD8: //CLD();
		flags&=(DEC_OFF);
		curr_inst=0x80;
		curr_loc++;
		break;
	case 0xD9: //CMP_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			CMP();
		}
		if(((address-y_reg)/256)!=(address/256))
		{
			if(curr_cycle++==4) curr_inst=0x80;
		}
		else if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0xDD: //CMP_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			CMP();
		}
		if(((address-x_reg)/256)!=(address/256))
		{
			if(curr_cycle++==4) curr_inst=0x80;
		}
		else if(curr_cycle++==3) curr_inst=0x80;
		break;
	case 0xDE: //DEC_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			Poke(address,gen1=Peek(address)-1);
			setflags(gen1);
		}
		if(curr_cycle++==6) curr_inst=0x80;
		break;
		
	case 0xE0: //CPX_Imm();
		Imm();
		CPX();
		curr_inst=0x80;
		break;
	case 0xE1: //SBC_IndX();
		if(curr_cycle==1)
		{
			IndX();
			SBC();
		}
		if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0xE4: //CPX_Zero();
		if(curr_cycle==1)
		{
			Zero();
			CPX();
		}
		if(curr_cycle++==2)
			curr_inst=0x80;
		break;
	case 0xE5: //SBC_Zero();
		if(curr_cycle==1)
		{
			Zero();
			SBC();
		}
		if(curr_cycle++==2)curr_inst=0x80;
		break;
	case 0xE6: //INC_Zero();
		if(curr_cycle==1)
		{
			Zero();
			Poke(address,gen1=Peek(address)+1);
			setflags(gen1);
		}
		if(curr_cycle++==4) curr_inst=0x80;
		break;

	case 0xE9: //SBC_Imm();
		Imm();
		SBC();
		curr_inst=0x80;
		break;
		
		// UNDOC'D
	case 0x1A:
	case 0x3A:
	case 0x5A:
	case 0x7A:
	case 0xDA:
	case 0xFA:
	case 0xEA: //NOP();
		curr_loc++;
		curr_inst=0x80;
		break;
	case 0xEC: //CPX_Abs();
		if(curr_cycle==1)
		{
			Abs();
			CPX();
		}
		if(curr_cycle++==3)
			curr_inst=0x80;
		break;
	case 0xED: //SBC_Abs();
		if(curr_cycle==1)
		{
			Abs();
			SBC();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0xEE: //INC_Abs();
		if(curr_cycle==1)
		{
			Abs();
			Poke(address,gen1=Peek(address)+1);
			setflags(gen1);
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0xF1: //SBC_Ind_Y();
		if(curr_cycle==1)
		{
			Ind_Y();
			SBC();
		}
		if(curr_cycle++==4)curr_inst=0x80;
		break;
	case 0xF5: //SBC_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			SBC();
		}
		if(curr_cycle++==3)curr_inst=0x80;
		break;
	case 0xF6: //INC_Zero_X();
		if(curr_cycle==1)
		{
			Zero_X();
			Poke(address,gen1=Peek(address)+1);
			setflags(gen1);
		}
		if(curr_cycle++==5) curr_inst=0x80;
		break;
	case 0xF8: //SED();
		flags|=FLAG_DECIMAL;
		curr_inst=0x80;
		curr_loc++;
		break;
	case 0xF9: //SBC_Abs_Y();
		if(curr_cycle==1)
		{
			Abs_Y();
			SBC();
		}
		if(((address-y_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0xFD: //SBC_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			SBC();
		}
		if(((address-x_reg)/256)==(address/256))
		{
			if(curr_cycle++==4)curr_inst=0x80;
		}
		else if(curr_cycle++==5)curr_inst=0x80;
		break;
	case 0xFE: //INC_Abs_X();
		if(curr_cycle==1)
		{
			Abs_X();
			Poke(address,gen1=Peek(address)+1);
			setflags(gen1);
		}
		if(curr_cycle++==6) curr_inst=0x80;
		break;
		
		
		// UNDOC'D INSTRUCTIONS ///////////
	case 0x82:	// SKB
	case 0xc2:
	case 0xe2:
	case 0x04:
	case 0x14:
	case 0x34:
	case 0x44:
	case 0x54:
	case 0x64:
	case 0x74:
	case 0x89: // op tests call it NOP #imm ??
	case 0xD4:
	case 0xf4:	curr_loc+=2;	curr_inst=0x80;	break;
#ifdef UNDOCd
	case 0x03: //ASO_Ind_X
		if(curr_cycle==1)
		{
			IndX();
			ASO();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0x07: //ASO_Zero
		if(curr_cycle==1)
		{
			Zero();
			ASO();
		}
		if(curr_cycle++==5)
			curr_inst=0x80;
		break;
#endif
	case 0x0B:	//ASO_Imm
		if(curr_cycle==1)
		{
			Imm();
			gen1=Peek(address);
			if(gen1&0x80)
				flags|=FLAG_CARRY;
			gen1<<=1;
			a_reg|=gen1;
			setflags(a_reg);
		}
		if(curr_cycle++==4)
			curr_inst=0x80;
		break;
#ifdef UNDOCd
	case 0x0F:	Abs(); ASO();	break;
	case 0x13:	Ind_Y(); ASO();	break;
	case 0x17:	//ASO_Zero_X
		if(curr_cycle==1)
		{
			Zero_X();
			ASO();
		}
		if(curr_cycle++==6)
			curr_inst=0x80;
		break;
	case 0x1B:	Abs_Y();	ASO();	break;
#endif
	case 0x1F:
		if(curr_cycle==1)
		{
			Abs_X();
			ASO();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
#ifdef UNDOCd
	case 0x23:	//RLA_IndX
		if(curr_cycle==1)
		{
			IndX();
			RLA();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0x27:	//RLA_Zero
		if(curr_cycle==1)
		{
			Zero();
			RLA();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0x2B:
		if(curr_cycle==1)
		{
			Imm();
			RLA();
		}
		if(curr_cycle++==6)
			curr_inst=0x80;
		break;
	case 0x2F:
		if(curr_cycle==1)
		{
			Abs();
			RLA();
		}
		if(curr_cycle++==7)
			curr_inst=0x80;
		break;
	case 0x33:
		if(curr_cycle==1)
		{
			Ind_Y();
			RLA();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0x37:
		if(curr_cycle==1)
		{
			Zero_X();
			RLA();
		}
		if(curr_cycle++==7)
			curr_inst=0x80;
		break;
	case 0x3B:
		if(curr_cycle==1)
		{
			Abs_Y();
			RLA();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0x3F:
		if(curr_cycle==1)
		{
			Abs_X();
			RLA();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
#endif
		//case 0x43:	IndX();	LSE();	break;
		//case 0x47:	Zero();	LSE();	break;
		//case 0x4B:	Imm();	ALR();	break;//
		//case 0x4F:	Abs();	LSE();	break;
		//case 0x53:	Ind_Y();	LSE();	break;
		//case 0x57:	Zero_X();	LSE();	break;
		//case 0x5B:	Abs_Y();LSE();	break;
		//case 0x5F:	Abs_X();	LSE();	break; //
		//case 0x63:	IndX();	RRA();	break;
	case 0x67:
		if(curr_cycle==1)
		{
			Zero();
			RRA();
		}
		if(curr_cycle++==5)
			curr_inst=0x80;
		break;
		//case 0x6B:	Imm();	ARR();	break; //
		//case 0x6F:	Abs();	RRA();	break;
		//case 0x73:	Ind_Y();	RRA();	break;
		//case 0x77:	Zero_X();	RRA();	break;
		//case 0x7B:	Abs_Y();	RRA();	break;
		//case 0x7F:	Abs_X();	RRA();	break;
		//case 0x83:	IndX();	AXS();	break;
	case 0x87:
		if(curr_cycle==1)
		{
			Zero();
			AXS();
		}
		if(curr_cycle++==5)
			curr_inst=0x80;
		break;
		//case 0x8F:	Abs();	AXS();	break;
		//case 0x97:	Zero_Y();	AXS();	break;
		//case 0xA3:	IndX();	LAX();	break;
		//case 0xA7:	Zero();	LAX();	break;
		//case 0xAB:	Imm();	OAL();	break;
		//case 0xAF:	Abs();	LAX();	break;
		//case 0xB3:	Ind_Y();	LAX();	break;
		//case 0xB7:	Zero_X();	LAX();	break;
	case 0xBF:
		if(curr_cycle==1)
		{
			Abs_Y();
			LAX();
		}
		if(curr_cycle==1)
			curr_inst=0x80;
		break;
	case 0xC3:
		if(curr_cycle==1)
		{
			IndX();
			DCM();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0xC7:
		if(curr_cycle==1)
		{
			Zero();
			DCM();
		}
		if(curr_cycle++==6)
			curr_inst=0x80;
		break;
		//case 0xCB:	Imm();		SAX();	break;
		//case 0xCF:	Abs();		DCM();	break;
		//case 0xD3:	Ind_Y();	DCM();	break;
		//case 0xD7:	Zero_X();	DCM();	break;
		//case 0xDB:	Abs_Y();	DCM();	break;
		//case 0xDF:	Abs_X();	DCM();	break;
		//case 0xE3:	IndX();		INS();	break;
		//case 0xE7:	Zero();		INS();	break;
		//case 0xEF:	Abs();		INS();	break;
		//case 0xF3:	Ind_Y();	INS();	break;
		//case 0xF7:	Zero_X();	INS();	break;
		//case 0xFB:	Abs_Y();	INS();	break;
	case 0xFF:
		if(curr_cycle==1)
		{
			Abs_X();
			INS();
		}
		if(curr_cycle++==8)
			curr_inst=0x80;
		break;
	case 0x0C:	// SKW
	case 0x1C:
	case 0x3C:
	case 0x5C:
	case 0x7C:
	case 0xDC:
	case 0xFC:	curr_loc+=3;	curr_inst=0x80;	break;
	default:
/*		if(illopc)
				Processor_hung(curr_loc,curr_inst,jmp_from);
*/		curr_loc++;
		curr_inst=0x80;
	}// end switch
	
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




