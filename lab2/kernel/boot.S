.section ".text.boot"

.global _start

// x0 register stores address to dtb
_start:

    ldr		x1, =_dtb_ptr	//put _dtb_ptr into register1
	str		x0, [x1]		//store dtb address from x0 to _dtb_ptr
    
    // clear bss
    ldr     x20, =_bss_start
    ldr     x21, =_bss_size
    add     x21, x21, x20
3:  
    cmp     x20, x21
    b.eq    4f
    str     xzr, [x20]
    add     x20, x20, #8
    b       3b

4:  
    // initialize sp
    ldr     x20, =_start
    mov     sp, x20

    // jump to C code, should not return
    bl      kernel_main
5:
    // for failsafe, halt this core too
    b       5b

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0