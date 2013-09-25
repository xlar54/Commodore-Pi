//
// Utility functions for interacting with the memory mapped IO peripherals
//

// C Function for the assembly instruction 
// str val, [addr]
// which stores the value of val in addr
static inline void mmio_write(unsigned int addr, unsigned int data)
{
	asm volatile("str %[data], [%[address]]" 
		: : [address]"r"((unsigned int*)addr), [data]"r"(data));
}

// C Function for the assembly instruction
// ldr val, [addr]
// Which loads the value at addr into val
static inline void unsigned int mmio_read(unsigned int addr)
{
	unsigned int data;
	asm volatile("ldr %[data], [%[addr]]"
		: [data]"=r"(data) : [addr]"r"((unsigned int*)addr));
		
	return data;
}