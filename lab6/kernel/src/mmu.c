#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"

void* set_2M_kernel_mmu(void* x0)
{
   // Turn
   //   Two-level Translation (1GB) - in boot.S
   // to
   //   Three-level Translation (2MB) - set PUD point to new table
   unsigned long *pud_table = (unsigned long *) MMU_PUD_ADDR;

    unsigned long *pte_table1 = (unsigned long *) MMU_PTE_ADDR;
    unsigned long *pte_table2 = (unsigned long *)(MMU_PTE_ADDR + 0x1000L);
    for (int i = 0; i < 512; i++)
    {
        unsigned long addr = 0x200000L * i;
        if ( addr >= PERIPHERAL_END )
        {
            pte_table1[i] = ( 0x00000000 + addr ) + BOOT_PTE_ATTR_nGnRnE;
            continue;
        }
        pte_table1[i] = (0x00000000 + addr ) + BOOT_PTE_ATTR_NOCACHE; //   0 * 2MB
        pte_table2[i] = (0x40000000 + addr ) + BOOT_PTE_ATTR_NOCACHE; // 512 * 2MB
    }

    // set PUD
    pud_table[0] = (unsigned long)pte_table1 + BOOT_PUD_ATTR;
    pud_table[1] = (unsigned long)pte_table2 + BOOT_PUD_ATTR;

    return x0;
}

void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag)
{
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        // PGD 47-39, PUD 30-38, PME 21-29, PTE 12-20
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff;

        if (level == 3) // arrive final(PTE)
        {
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_UK_ACCESS | PD_KNX | flag;
            return;
        }

        if(!table_p[idx]) // if table not exist
        {
            size_t* newtable_p =kmalloc(0x1000); // create a table
            memset(newtable_p, 0, 0x1000);
            table_p[idx] = VIRT_TO_PHYS((size_t)newtable_p); // point to that table
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | flag;
        }

        // go to next level
        table_p = (size_t*)PHYS_TO_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK));
    }
}

void mappages(size_t *virt_pgd_p, size_t va, size_t size, size_t pa, size_t flag)
{
    pa = pa - (pa % 0x1000); // align
    for (size_t s = 0; s < size; s+=0x1000)
    {
        map_one_page(virt_pgd_p, va + s, pa + s, flag);
    }
}

void free_page_tables(size_t *page_table, int level)
{
    size_t *table_virt = (size_t*)PHYS_TO_VIRT((char*)page_table);
    for (int i = 0; i < 512; i++)
    {
        // 從PGD開始檢查
        if (table_virt[i] != 0)
        {
            size_t *next_table = (size_t*)(table_virt[i] & ENTRY_ADDR_MASK);
            if (table_virt[i] & PD_TABLE)
            { 
                // 0(PGD) -> 1(PUD) -> 2(PMD)
                if(level!=2)free_page_tables(next_table, level + 1);
                table_virt[i] = 0L;
                kfree(PHYS_TO_VIRT((char *)next_table));
            }
        }
    }
}