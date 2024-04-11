#ifndef __REGS_H__
#define __REGS_H__

#define CPACR_EL1_FPEN (0b11 << 20)
#define get_reg(var_name, target)             \
    asm volatile("mrs    %0, " #target "\n\t" \
                 : "=r"(var_name))

#endif