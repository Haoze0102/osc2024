/* entry point */
ENTRY(_start)

SECTIONS
{
    _text_start = .;
    .text : 
    { 
        *(.text) 
    }
    . = ALIGN(0x1000); /* align to page size */
}