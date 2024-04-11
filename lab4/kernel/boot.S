#include "regs.h"

.section ".text.boot"

.global _start

// x0 register stores address to dtb
_start:

    ldr		x1, =_dtb_ptr	//put _dtb_ptr into register1
	str		x0, [x1]		//store dtb address from x0 to _dtb_ptr

set_exception_vector_table:
    adr     x1, Vector_table_el1
    msr     vbar_el1, x1

from_el2_to_el1:
    mov     x1, (1 << 31) // EL1 uses aarch64
    msr     hcr_el2, x1
    mov     x1, 0x3c5
    msr     spsr_el2, x1
    ldr     x1, =el1_start // return to `el1_start` when goto EL1
    msr     elr_el2, x1
    eret

el1_start:
    // enable all instructions -> set FPEN, bits [21:20] to 0b11 at CPACR_EL1 
    // This control does not cause execution of SIMD and floating-point instructions to be trapped.
    ldr     x1, =CPACR_EL1_FPEN
    msr     cpacr_el1, x1

    // initialize EL1 stack
    ldr     x1, =_el1_stack_end
    mov     sp, x1 

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


.section ".vector_table"

// save general registers to stack
.macro save_all
    sub     sp, sp, 32 * 8
    stp     x0, x1, [sp ,16 * 0]
    stp     x2, x3, [sp ,16 * 1]
    stp     x4, x5, [sp ,16 * 2]
    stp     x6, x7, [sp ,16 * 3]
    stp     x8, x9, [sp ,16 * 4]
    stp     x10, x11, [sp ,16 * 5]
    stp     x12, x13, [sp ,16 * 6]
    stp     x14, x15, [sp ,16 * 7]
    stp     x16, x17, [sp ,16 * 8]
    stp     x18, x19, [sp ,16 * 9]
    stp     x20, x21, [sp ,16 * 10]
    stp     x22, x23, [sp ,16 * 11]
    stp     x24, x25, [sp ,16 * 12]
    stp     x26, x27, [sp ,16 * 13]
    stp     x28, x29, [sp ,16 * 14]
    str     x30, [sp, 16 * 15]
.endm

// load general registers from stack
.macro load_all
    ldp     x0, x1, [sp ,16 * 0]
    ldp     x2, x3, [sp ,16 * 1]
    ldp     x4, x5, [sp ,16 * 2]
    ldp     x6, x7, [sp ,16 * 3]
    ldp     x8, x9, [sp ,16 * 4]
    ldp     x10, x11, [sp ,16 * 5]
    ldp     x12, x13, [sp ,16 * 6]
    ldp     x14, x15, [sp ,16 * 7]
    ldp     x16, x17, [sp ,16 * 8]
    ldp     x18, x19, [sp ,16 * 9]
    ldp     x20, x21, [sp ,16 * 10]
    ldp     x22, x23, [sp ,16 * 11]
    ldp     x24, x25, [sp ,16 * 12]
    ldp     x26, x27, [sp ,16 * 13]
    ldp     x28, x29, [sp ,16 * 14]
    ldr     x30, [sp, 16 * 15]
    add     sp, sp, 32 * 8
.endm


// Typical exception vector table code.
.balign 0x800
Vector_table_el1:
curr_el_sp0_sync:        // The exception handler for a synchronous 
                         // exception from the current EL using SP0.
    mov     x0, #0
    b       _invalid_exception

.balign 0x80
curr_el_sp0_irq:         // The exception handler for an IRQ exception
                         // from the current EL using SP0.
    mov     x0, #1
    b       _invalid_exception

.balign 0x80
curr_el_sp0_fiq:         // The exception handler for an FIQ exception
                         // from the current EL using SP0.
    mov     x0, #2
    b       _invalid_exception

.balign 0x80
curr_el_sp0_serror:      // The exception handler for a System Error 
                         // exception from the current EL using SP0.
    mov     x0, #3
    b       _invalid_exception

.balign 0x80
curr_el_spx_sync:        // The exception handler for a synchrous 
                         // exception from the current EL using the
                         // current SP.
    mov     x0, #4
    b       _invalid_exception

.balign 0x80
curr_el_spx_irq:         // The exception handler for an IRQ exception from 
                         // the current EL using the current SP.
    b       _lower_el_aarch64_irq                     

.balign 0x80
curr_el_spx_fiq:         // The exception handler for an FIQ from 
                         // the current EL using the current SP.
    mov     x0, #6
    b       _invalid_exception                       

.balign 0x80
curr_el_spx_serror:      // The exception handler for a System Error 
                         // exception from the current EL using the
                         // current SP.
    mov     x0, #7
    b       _invalid_exception                        

.balign 0x80
lower_el_aarch64_sync:   // The exception handler for a synchronous 
                         // exception from a lower EL (AArch64).
    b       _lower_el_aarch64_sync

.balign 0x80
lower_el_aarch64_irq:    // The exception handler for an IRQ from a lower EL
                         // (AArch64).
    b       _lower_el_aarch64_irq                         

.balign 0x80
lower_el_aarch64_fiq:    // The exception handler for an FIQ from a lower EL
                         // (AArch64).
    mov     x0, #10
    b       _invalid_exception                         

.balign 0x80
lower_el_aarch64_serror: // The exception handler for a System Error 
                         // exception from a lower EL(AArch64).
    mov     x0, #11
    b       _invalid_exception                      

.balign 0x80
lower_el_aarch32_sync:   // The exception handler for a synchronous 
                         // exception from a lower EL(AArch32).
    mov     x0, #12
    b       _invalid_exception

.balign 0x80
lower_el_aarch32_irq:    // The exception handler for an IRQ exception 
                         // from a lower EL (AArch32).
    mov     x0, #13
    b       _invalid_exception

.balign 0x80
lower_el_aarch32_fiq:    // The exception handler for an FIQ exception from 
                         // a lower EL (AArch32).
    mov     x0, #14
    b       _invalid_exception                        

.balign 0x80
lower_el_aarch32_serror: // The exception handler for a System Error
                         // exception from a lower EL(AArch32).
    mov     x0, #15
    b       _invalid_exception                          

_lower_el_aarch64_sync:
    save_all
    bl      sync_handler   
    load_all
    eret 

_lower_el_aarch64_irq:
    save_all
    bl      irq_handler
    load_all
    eret

_invalid_exception:
    save_all
    bl      invalid_handler
    load_all
    eret

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0