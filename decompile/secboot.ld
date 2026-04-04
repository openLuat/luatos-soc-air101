/**
 * secboot.ld - Linker script for reconstructed secboot.bin
 *
 * Architecture: C-SKY XT804 (CK804EF)
 * Load Address: 0x08002400
 *
 * Memory layout matches original secboot.bin:
 *   Flash: 0x08002400 - 0x080077DB (21,468 bytes)
 *   SRAM:  0x20000000 - 0x20028000
 */

MEMORY
{
    /* Flash partition for secboot (within internal flash) */
    FLASH (rx)  : ORIGIN = 0x08002400, LENGTH = 64K

    /* SRAM: Data + BSS + Heap */
    SRAM  (rwx) : ORIGIN = 0x20010000, LENGTH = 96K
}

/* Entry point */
ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table at the very start of flash */
    .vectors 0x08002400 :
    {
        KEEP(*(.vectors))
    } > FLASH

    /* Code section */
    .text :
    {
        . = ALIGN(4);
        *(.text.pre_reset_hook)
        *(.text.Reset_Handler)
        *(.text.Default_Handler)
        *(.text.tspend_handler)

        /* Standard library and memory functions (low addresses in original) */
        *secboot_stdlib.o(.text .text.*)
        *secboot_memory.o(.text .text.*)

        /* Flash operations */
        *secboot_flash.o(.text .text.*)

        /* Crypto operations */
        *secboot_crypto.o(.text .text.*)

        /* Image validation */
        *secboot_image.o(.text .text.*)

        /* Firmware update */
        *secboot_fwup.o(.text .text.*)

        /* Boot logic */
        *secboot_boot.o(.text .text.*)

        /* UART driver */
        *secboot_uart.o(.text .text.*)

        /* Hardware init and main */
        *secboot_hw_init.o(.text .text.*)
        *secboot_main.o(.text .text.*)

        /* Catch-all for any remaining code sections */
        *(.text .text.*)
        . = ALIGN(4);
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        . = ALIGN(4);
        __erodata = .;
    } > FLASH

    /* Initialized data (loaded from flash, copied to SRAM at startup) */
    .data : AT(__erodata)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        __data_end__ = .;
    } > SRAM

    /* Uninitialized data (zeroed at startup) */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > SRAM

    /* Heap area */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start = .;
        . = . + 0x16BD0;   /* ~93KB heap */
        __heap_end = .;
    } > SRAM

    /* Discard debug info and other unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
        *(.debug*)
    }
}
