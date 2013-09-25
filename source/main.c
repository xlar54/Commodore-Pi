#include "gpio.h"
#include "timer.h"
#include "terminal.h"
#include "string.h"
#include "usbd/usbd.h"
#include "keyboard.h"

extern unsigned int gPitch;

void OnCriticalError(void)
{
	while(1)
	{
		LedOff();
		
		wait(1000);		
		
		LedOn();		
		
		wait(1000);
	}
}

// Log function for CSUD
void LogPrint(char* message, unsigned int length)
{
	//print(message, length);
}

int cmain2(void)
{
	//No keyboard
	unsigned int result = 0;
	
	LedInit();
	
	if((result = terminal_init()) != 0)
		OnCriticalError();

	CB64_MainLoop();

}

int cmain(void)
{
	unsigned int result = 0;
	
	LedInit();
	
	if((result = terminal_init()) != 0)
		OnCriticalError();

	printf("\n\n     One moment...");
		
	if((result = UsbInitialise()) != 0)
		printf("Usb initialise failed, error code: %d\n", result);
	else
	{
		if((result = KeyboardInitialise()) != 0)
			printf("Keyboard initialise failed, error code: %d\n", result);
		else
			CB64_MainLoop();
	}
	
}

int cmainx(void)
{
	unsigned int result = 0;
	int fb = 1;
	
	LedInit();
	
	if((result = terminal_init()) != 0)
		OnCriticalError();

	if((result = UsbInitialise()) != 0)
		printf("Usb initialise failed, error code: %d\n", result);
	else
	{
		if((result = KeyboardInitialise()) != 0)
			printf("Keyboard initialise failed, error code: %d\n", result);
		else
		{
			//Draw on second buffer
			SetVirtualFrameBuffer(2);
			terminal_clear();
			printf("This is framebuffer 2");
			
			SetVirtualFrameBuffer(1);
			terminal_clear();
			printf("\nFramebuffer 1 - Keyboard test\n\n");
			printf("gPitch= %d\n\n", gPitch);

			while(1)
			{
				KeyboardUpdate();
				
				short scanCode = KeyboardGetChar();		
				
				// Nothing pressed
				if(scanCode == 0)
				{
					wait(10);
					continue;
				}
				
				virtualkey vk = ScanToVirtual(scanCode);
				
				char c = VirtualToAsci(vk, KeyboardShiftDown());
				
				//printf("%c", c);
				
				char name[15];
				char* keyname = GetKeyName(name, 15, vk);
				
				printf("Scan: %d Vk: %d Name: %s\n", scanCode, (unsigned int)vk, keyname);

				wait(10);
				
				if(c == 65)
				{	
					if(fb == 1)
						fb=2;
					else
						fb=1;
					
					SetVirtualFrameBuffer(fb);
					DisplayVirtualFramebuffer(fb);
				}
				
			}
		}

	}
	
}




