#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

static volatile unsigned int *MAILBOX0READ = (unsigned int *)(0x2000b880);
static volatile unsigned int *MAILBOX0STATUS = (unsigned int *)(0x2000b898);
static volatile unsigned int *MAILBOX0WRITE = (unsigned int *)(0x2000b8a0);

unsigned int Mailbox_Read(unsigned int channel)
{
	unsigned int count = 0;
	unsigned int data;

	// Loop until something is received on the channel
	while(1)
	{
		while (*MAILBOX0STATUS & MAILBOX_EMPTY)
		{
			// Arbitrary large number for timeout
			if(count++ >(1<<25))
			{
				return 0xffffffff;
			}
		}
		
		data = *MAILBOX0READ;

		if ((data & 15) == channel)
			return data;
	}
}

void Mailbox_Write(unsigned int channel, unsigned int data)
{
	// Wait until there's space in the mailbox
	while (*MAILBOX0STATUS & MAILBOX_FULL){
	}
	
	// 28 MSB is data, 4 LSB = channel
	*MAILBOX0WRITE = (data | channel);
}
