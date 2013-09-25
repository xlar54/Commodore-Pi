#include "framebuffer.h"
#include "mailbox.h"

unsigned int gFbAddr;
unsigned int gPitch;
unsigned int gScreenWidth, gScreenHeight;


int InitializeFramebuffer()
{	
	unsigned int result = 0;
	
	if((result = GetScreenSizeFromTags()) > 0)
	{
		return result;
	}
	
	if((result = SetupScreen()) > 0)
	{
		return result;
	}
	
	if((result = GetPitch()) > 0)
	{
		return result;
	}
	
	return result;
}

// 0: Success. 1: Invalid response to property request, 2: Invalid screen size returned
int GetScreenSizeFromTags()
{
	volatile unsigned int mailbuffer[256] __attribute__ ((aligned (16)));
	unsigned int mailbufferAddr = (unsigned int)mailbuffer;
	
	mailbuffer[0] = 8 * 4;		// Total size
	mailbuffer[1] = 0;			// Request
	mailbuffer[2] = 0x40003;	// Display size
	mailbuffer[3] = 8;			// Buffer size
	mailbuffer[4] = 0;			// Request size
	mailbuffer[5] = 0;			// Space for horizontal resolution
	mailbuffer[6] = 0;			// Space for vertical resolution
	mailbuffer[7] = 0;			// End tag
	
	Mailbox_Write(8, mailbufferAddr);
	
	Mailbox_Read(8);
	
	if(mailbuffer[1] != 0x80000000)
		return 1;
		
	gScreenWidth = mailbuffer[5];
	gScreenHeight = mailbuffer[6];
		
	if(gScreenWidth == 0 || gScreenHeight == 0)
		return 2;
		
	return 0;
}

// 0: Success, 1: Invalid response to Setup screen request, 2: Framebuffer setup failed, Invalid tags, 3: Invalid tag response, 4: Invalid tag data
int SetupScreen()
{
	volatile unsigned int mailbuffer[256] __attribute__ ((aligned (16)));
	unsigned int mailbufferAddr = (unsigned int)mailbuffer;
	
	mailbuffer[0] = 8 * 4; // NOT SURE IF WE NEED THIS
	
	unsigned int c = 1;
	mailbuffer[c++] = 0;			 // This is a request
	mailbuffer[c++] = 0x00048003;	 // Tag id (set physical size)
	mailbuffer[c++] = 8;			 // Value buffer size (bytes)
	mailbuffer[c++] = 8;			 // Req. + value length (bytes)
	mailbuffer[c++] = SCREEN_WIDTH;  // Horizontal resolution
	mailbuffer[c++] = SCREEN_HEIGHT; // Vertical resolution

	mailbuffer[c++] = 0x00048004;	 // Tag id (set virtual size)
	mailbuffer[c++] = 8;			 // Value buffer size (bytes)
	mailbuffer[c++] = 8;			 // Req. + value length (bytes)
	mailbuffer[c++] = SCREEN_WIDTH;	 // Horizontal resolution
	mailbuffer[c++] = SCREEN_HEIGHT*2; // Vertical resolution - x2 allows for a second virtual framebuffer

	mailbuffer[c++] = 0x00048005;	 // Tag id (set depth)
	mailbuffer[c++] = 4;		     // Value buffer size (bytes)
	mailbuffer[c++] = 4;			 // Req. + value length (bytes)
	mailbuffer[c++] = 16;			 // 16 bpp

	mailbuffer[c++] = 0x00040001;	 // Tag id (allocate framebuffer)
	mailbuffer[c++] = 8;			 // Value buffer size (bytes)
	mailbuffer[c++] = 4;			 // Req. + value length (bytes)
	mailbuffer[c++] = 16;			 // Alignment = 16
	mailbuffer[c++] = 0;			 // Space for response

	mailbuffer[c++] = 0;			 // Terminating tag

	mailbuffer[0] = c*4;			 // Buffer size

	Mailbox_Write(8, mailbufferAddr);
	
	Mailbox_Read(8);
	
	if(mailbuffer[1] != 0x80000000)
		return 1;
		
	unsigned int temp;
	unsigned int count = 2; // Read the first tag
	while((temp = mailbuffer[count]))
	{
		if(temp == 0x40001)
			break;
			
		count += 3 + (mailbuffer[count + 1] >> 2);		
		if(count > c)
			return 2; // Framebuffer setup failed, Invalid tags.
	}
	
	// 8 bytes, plus the MSB is set to indicate that this is a response
	if(mailbuffer[count + 2] != 0x80000008)
		return 3; // Invalid tag response
		
	gFbAddr = mailbuffer[count + 3];
	unsigned int screenSize = mailbuffer[count + 4];
	
	if(gFbAddr == 0 || screenSize == 0)
		return 4;
		
	return 0;
}

// 0: Success, 1: Invalid pitch response, 2: Invalid pitch response
int GetPitch()
{
	volatile unsigned int mailbuffer[256] __attribute__ ((aligned (16)));
	unsigned int mailbufferAddr = (unsigned int)mailbuffer;
	
	// All super so far - Now time to get the pitch (bytes per line)
	mailbuffer[0] = 7 * 4; 		// Total Size
	mailbuffer[1] = 0; 			// This is a request
	mailbuffer[2] = 0x40008;	// Display size
	mailbuffer[3] = 4; 			// Buffer size
	mailbuffer[4] = 0;			// Request size
	mailbuffer[5] = 0; 			// REPONSE - Pitch
	mailbuffer[6] = 0;			// end tag
	
	Mailbox_Write(8, mailbufferAddr);
	
	Mailbox_Read(8);

	// 4 bytes, plus the MSB set to indicate a response
	if(mailbuffer[4] != 0x80000004)
		return 1; // Invalid pitch response
					
	unsigned int pitch = mailbuffer[5];
	if(pitch == 0)
		return 2; // Invalid pitch response
		
	gPitch = pitch;
	
	return 0;
}

void DoFlipy( void ) 
{
	// Copy all the things from one buffer to the other.
	unsigned int ctr = gScreenWidth * gScreenHeight;

	unsigned short int* onscreenBuffer = (unsigned short int*)gFbAddr;
	unsigned short int* offscreenBuffer = (unsigned short int*)(gFbAddr + (ctr * sizeof(unsigned short int) ) - gScreenWidth);
				
	while(ctr--)
		*onscreenBuffer++ = *offscreenBuffer++;
}
void DoFlipx( void ) 
{
	int x, y;
	unsigned short int* a;
	unsigned short int* b;

	for ( y = 0; y < gScreenHeight; y++ ) {
		for ( x = 0; x < gScreenWidth; x++ ) {

			// Do a double array copy.
			a = (unsigned short int*)( gFbAddr + ( y * gPitch ) + (x * 2) );
			b = (unsigned short int*)( gFbAddr+ ( ( y + gScreenHeight ) * gPitch ) + (x * 2) );

			*a = *b;
		}	
	}
}

void DisplayVirtualFramebuffer(int bufferId)
{
	volatile unsigned int mailbuffer[256] __attribute__ ((aligned (16)));
	unsigned int mailbufferAddr = (unsigned int)mailbuffer;
	
	mailbuffer[0] = 8 * 4;		// Total size
	mailbuffer[1] = 0;			// Request
	mailbuffer[2] = 0x48009;	// Display size
	mailbuffer[3] = 8;			// Buffer size
	mailbuffer[4] = 8;									// Request size
	mailbuffer[5] = SCREEN_WIDTH;						// Space for horizontal resolution
	mailbuffer[6] = SCREEN_HEIGHT * bufferId;		// Space for vertical resolution
	mailbuffer[7] = 0;			// End tag
	
	Mailbox_Write(8, mailbufferAddr);
	
	Mailbox_Read(8);
	
	// We should do some kind of error handling
	if(mailbuffer[1] != 0x80000000)
		return;
	
	return;
}