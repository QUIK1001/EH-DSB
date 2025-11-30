ENTRY(kernel_main)
SECTIONS
{
    . = 0x10000;
    .text : {
        *(.text*)
    }
    .data : {
        *(.data*)
        *(.rodata*)
    }
    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    }
}
