.globl _start
_start:

	mov sp, #0x8000
	
;@ Clear out bss.
	ldr	r4, =_bss_start
	ldr	r9, =_bss_end
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r8, #0
        b       2f
 
1:
	;@ store multiple at r4.
	stmia	r4!, {r5-r8}
 
	;@ If we are still below bss_end, loop.
2:
	cmp	r4, r9
	blo	1b
	
	bl cmain

hang: 
	b hang

.globl PUT32
PUT32:
	str r1, [r0]
	bx lr

.globl GET32
GET32:
	ldr r0, [r0]
	bx lr
	
.globl dummy
dummy:
	bx lr
