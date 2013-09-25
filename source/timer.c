extern unsigned int GET32(unsigned int);

#define SYSTIMER_COUNTER 0x20003004

void wait(unsigned int milliSeconds)
{
	unsigned int ttw = 1048 * milliSeconds;
	unsigned int start = GET32(SYSTIMER_COUNTER);
	while(1)
	{
		if(GET32(SYSTIMER_COUNTER) - start >= ttw)
			break;
	}
}
