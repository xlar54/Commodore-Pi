#define FLAG_NEGATIVE 128
#define FLAG_OVERFLOW 64
#define FLAG_BREAK    16
#define FLAG_DECIMAL  8
#define FLAG_INT      4
#define FLAG_ZERO     2
#define FLAG_CARRY    1

#define NEG_OFF  127
#define OVER_OFF 191
#define BRK_OFF  239
#define DEC_OFF  247
#define INT_OFF  251
#define ZERO_OFF 253
#define CRRY_OFF 254



void CB64_6510();

#define setflags(r) { \
	if(r&0x80)\
	 {\
	  flags|=FLAG_NEGATIVE;\
	 }\
	else\
	 {\
	  flags&=(NEG_OFF);\
	 }\
	if(!r)flags|=FLAG_ZERO;else flags&=(ZERO_OFF);\
}
#define IndX(){\
	address=(BYTE)(Peek(curr_loc+1)+x_reg);\
	address=Deek(address);\
	curr_loc+=2;\
}
#define Zero(){\
	address=(WORD)Peek(curr_loc+1);\
	curr_loc+=2;\
}
#define Imm(){\
	address=++curr_loc;\
	curr_loc++;\
}
#define Abs(){\
	address=(WORD)Peek(++curr_loc);\
	address+=((WORD)Peek(++curr_loc)<<8);\
	curr_loc++;\
}
#define Ind_Y(){\
	address=(WORD)Peek(curr_loc+1);\
	address=Deek(address)+(WORD)y_reg;\
	curr_loc+=2;\
}
#define Zero_X(){\
	address=(BYTE)(Peek(curr_loc+1)+x_reg);\
	curr_loc+=2;\
}
#define Zero_Y(){\
	address=(BYTE)(Peek(curr_loc+1)+y_reg);\
	curr_loc+=2;\
}
#define Abs_Y(){\
	address=Deek(curr_loc+1)+y_reg;\
	curr_loc+=3;\
}
#define Abs_X(){\
	address=Deek(curr_loc+1)+x_reg;\
	curr_loc+=3;\
}

#define ORA(){\
	a_reg|=Peek(address);\
	setflags(a_reg);\
}

//ASL////////////////////////

#define ASL(){\
	if(Peek(address)&0x80)flags|=FLAG_CARRY;else flags&=CRRY_OFF;\
	Poke(address,gen1=Peek(address)<<1);\
	setflags(gen1);\
}

//AND/////////////////////////
#define AND(){\
	a_reg&=Peek(address);\
	setflags(a_reg);\
}

//BIT//////////////////////////
#define BIT(){\
	gen1=Peek(address);\
	K.B.l=a_reg&gen1;\
	setflags(K.B.l);\
	if(gen1&0x80)flags|=FLAG_NEGATIVE;else flags&=(NEG_OFF);\
	if(gen1&0x40)flags|=FLAG_OVERFLOW;else flags&=(OVER_OFF);\
}

//ROL//////////////////////////
#define ROL(){\
	gen1=Peek(address)<<1;\
	if(flags&FLAG_CARRY)gen1|=1;\
	if(Peek(address)&0x80)\
		flags|=FLAG_CARRY;\
	else\
		flags&=CRRY_OFF;\
	setflags(gen1);\
	Poke(address,gen1);\
}

//EOR//////////////////////////
#define EOR(){\
	a_reg^=Peek(address);\
	setflags(a_reg);\
}

//LSR////////////////////////
#define LSR(){\
	gen1=Peek(address);\
	if(gen1&1)flags|=FLAG_CARRY;else flags&=(CRRY_OFF);\
	gen1>>=1;\
	setflags(gen1);\
	Poke(address,gen1);\
}

//ADC////////////////////////		Thanks to M6502
//					NOT original M6502 code
#define ADC(){ \
	gen1=Peek(address); \
	if(flags&FLAG_DECIMAL) \
	{ \
		K.W=(a_reg&0xF)+(gen1&0xF)+(flags&FLAG_CARRY); \
		if(K.B.l>=0xA) \
			K.B.l+=0x6; \
		if(K.B.l>=0x20) \
			K.B.l=(K.B.l&0xF)+0x10; \
		K.W+=(a_reg&0xF0)+(gen1&0xF0); \
		flags&=~(FLAG_ZERO|FLAG_NEGATIVE|FLAG_OVERFLOW); \
		if(K.B.l&0x80) \
			flags|=FLAG_NEGATIVE; \
		if((BYTE)(a_reg+gen1+(flags&FLAG_CARRY))==0) \
			flags|=FLAG_ZERO; \
		if(~(a_reg^gen1)&(a_reg^K.B.l)&0x80) \
			flags|=FLAG_OVERFLOW; \
		if(K.W>=0xA0) \
		{ \
			flags|=FLAG_CARRY; \
			K.W+=0x60; \
		} \
		else \
			flags&=CRRY_OFF; \
		a_reg=K.B.l; \
	} \
	else \
	{ \
		K.W=a_reg+gen1+(flags&FLAG_CARRY); \
		flags&=~(FLAG_NEGATIVE|FLAG_CARRY|FLAG_ZERO|FLAG_OVERFLOW); \
		if(K.B.l&0x80) \
			flags|=FLAG_NEGATIVE; \
		if(!K.B.l) \
			flags|=FLAG_ZERO; \
		if(~(a_reg^gen1)&(a_reg^K.B.l)&0x80) \
			flags|=FLAG_OVERFLOW; \
		if(K.B.h) \
			flags|=FLAG_CARRY; \
		a_reg=K.B.l; \
	} \
}


//ROR//////////////////////////
#define ROR(){\
	gen1=Peek(address)>>1;\
	if(flags&FLAG_CARRY)gen1|=0x80;\
	if(Peek(address)&1)\
		flags|=FLAG_CARRY;\
	else\
		flags&=CRRY_OFF;\
	setflags(gen1);\
	Poke(address,gen1);\
}

//CPY////////////////////////
#define CPY(){\
	gen1=Peek(address);\
	if(gen1<=y_reg)flags|=FLAG_CARRY;\
		else flags&=(CRRY_OFF);\
	K.B.l=y_reg-gen1;\
	setflags(K.B.l);\
}

//CMP////////////////////////
#define CMP(){\
	gen1=Peek(address);\
	K.B.l=a_reg-gen1;\
	setflags(K.B.l);\
	if(gen1<=a_reg)flags|=FLAG_CARRY;\
		else flags&=(CRRY_OFF);\
}

//CPX//////////////////////////
#define CPX(){\
	gen1=Peek(address);\
	K.B.l=x_reg-gen1;\
	setflags(K.B.l);\
	if(gen1<=x_reg)flags|=FLAG_CARRY;\
		else flags&=(CRRY_OFF);\
}

//SBC//////////////////////////
//
#define SBC(){\
	gen1=Peek(address); \
	if(flags&FLAG_DECIMAL) \
	{ \
		K.W=(a_reg&0xF)+0x100; \
		K.W-=(gen1&0xF)+(~flags&FLAG_CARRY); \
		if(K.W<0x100) \
			K.W-=6; \
		if(K.W<0xF0) \
			K.W+=0x10; \
		K.W+=a_reg&0xF0; \
		K.W-=gen1&0xF0; \
		flags&=~(FLAG_ZERO|FLAG_NEGATIVE|FLAG_OVERFLOW); \
		if(K.B.l&0x80) \
			flags|=FLAG_NEGATIVE; \
		if((BYTE)(a_reg-gen1-(~flags&FLAG_CARRY))==0) \
			flags|=FLAG_ZERO; \
		if(~(K.B.l^gen1)&(a_reg^gen1)&0x80) \
			flags|=FLAG_OVERFLOW; \
		if(K.B.h) \
			flags|=FLAG_CARRY; \
		else \
		{ \
			K.B.l-=0x60; \
			flags&=CRRY_OFF; \
		} \
		a_reg=K.B.l; \
	} \
	else \
	{ \
		K.B.l=a_reg; \
		K.B.h=1; \
		K.W-=gen1+(~flags&FLAG_CARRY); \
		flags&=~(FLAG_NEGATIVE|FLAG_CARRY|FLAG_ZERO|FLAG_OVERFLOW); \
		if(K.B.l&0x80) \
			flags|=FLAG_NEGATIVE; \
		if(!K.B.l) \
			flags|=FLAG_ZERO; \
		if(~(K.B.l^gen1)&(a_reg^gen1)&0x80) \
			flags|=FLAG_OVERFLOW; \
		if(K.B.h) \
			flags|=FLAG_CARRY; \
		a_reg=K.B.l; \
	} \
}


// UNDOC'D INSTRUCTIONS ///////////////////
#define ASO(){\
	ASL();\
	ORA();\
}
#define RLA(){\
	ROL();\
	AND();\
}
#define LSE(){\
	LSR();\
	EOR();\
}
#define RRA(){\
	ROR();\
	ADC();\
}
#define AXS(){\
	Poke(address,a_reg&x_reg);\
}
#define LAX(){\
		a_reg=x_reg=Peek(address);\
		setflags(a_reg);\
}
#define DCM(){\
	Poke(address,gen1=Peek(address)-1);\
	setflags(gen1);\
	CMP();\
}
#define INS(){\
	Poke(address,Peek(address)+1);\
	SBC();\
}
#define ALR(){\
	AND();\
	if(a_reg&1)flags|=FLAG_CARRY;else flags&=(CRRY_OFF);\
	a_reg>>=1;\
	setflags(a_reg);\
}
#define ARR(){\
	AND();\
	gen1=a_reg>>1;\
	if(flags&FLAG_CARRY)gen1|=0x80;\
	if(a_reg&1)\
		flags|=FLAG_CARRY;\
	else\
		flags&=CRRY_OFF;\
	setflags(gen1);\
	a_reg=gen1;\
}
#define OAL(){\
	a_reg|=0xEE;\
	setflags(a_reg);\
	AND();\
	x_reg=a_reg;\
	setflags(x_reg);\
}
#define SAX(){\
	gen1=Peek(address);\
	x_reg&=a_reg;\
	if(flags&FLAG_DECIMAL) \
	{ \
		K.B.l=(x_reg&0x0F)-(gen1&0x0F)-(~flags&FLAG_CARRY); \
		if(K.B.l&0x10) K.B.l-=6; \
		K.B.h=(x_reg>>4)-(gen1>>4)-(K.B.l&0x10); \
		if(K.B.h&0x10) K.B.h-=6; \
		x_reg=(K.B.l&0x0F)|(K.B.h<<4); \
		if(K.B.h>15) \
			flags|=FLAG_CARRY; \
		else \
			flags&=CRRY_OFF; \
	} \
	else \
	{ \
		K.W=x_reg-gen1-(~flags&FLAG_CARRY); \
		flags&=~(FLAG_NEGATIVE|FLAG_OVERFLOW|FLAG_ZERO|FLAG_CARRY); \
		flags|=((x_reg^gen1)&(x_reg^K.B.l)&0x80? FLAG_OVERFLOW:0)| \
		(K.B.h? 0:FLAG_CARRY)|(K.B.l?0:FLAG_ZERO)|(K.B.l&0x80?FLAG_NEGATIVE:0); \
		x_reg=K.B.l; \
	}\
}
