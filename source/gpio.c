extern unsigned int GET32(unsigned int);
extern void PUT32(unsigned int, unsigned int);

#define GPFSEL1 0x20200004
#define GPSET0	 0x2020001C
#define GPCLR0	 0x20200028

void LedInit()
{
	// Enable output on the LED
	unsigned int ra = GET32(GPFSEL1); 
	
	ra &= ~(7 << 18);
	ra |= 1 << 18;
	
	PUT32(GPFSEL1, ra);
}

void LedOn()
{
	PUT32(GPCLR0, 1 << 16); // On
}

void LedOff()
{
	PUT32(GPSET0, 1 << 16); // Off
}
