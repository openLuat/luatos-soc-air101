/**
 * secboot_vectors.S - Reconstructed vector table and startup code
 *
 * Architecture: C-SKY XT804 (CK804EF)
 * Load Address: 0x08002400
 *
 * This is a reconstruction of the secboot.bin vector table and
 * Reset_Handler based on disassembly analysis.
 *
 * Cross-reference: platform/arch/xt804/bsp/startup.S
 */

/* ============================================================
 * Vector Table - 64 entries at 0x08002400
 * Each entry is a 4-byte absolute address (little-endian)
 * ============================================================ */
    .section .vectors
    .align  10
    .globl  __Vectors
    .type   __Vectors, @object
__Vectors:
    .long   Reset_Handler           /* [ 0] 0x08002506 - Entry point */
    .long   Default_Handler         /* [ 1] 0x080025B4 */
    .long   Default_Handler         /* [ 2] 0x080025B4 */
    .long   Default_Handler         /* [ 3] 0x080025B4 */
    .long   Default_Handler         /* [ 4] 0x080025B4 */
    .long   Default_Handler         /* [ 5] 0x080025B4 */
    .long   Default_Handler         /* [ 6] 0x080025B4 */
    .long   Default_Handler         /* [ 7] 0x080025B4 */
    .long   Default_Handler         /* [ 8] 0x080025B4 */
    .long   Default_Handler         /* [ 9] 0x080025B4 */
    .long   Default_Handler         /* [10] 0x080025B4 */
    .long   Default_Handler         /* [11] 0x080025B4 */
    .long   Default_Handler         /* [12] 0x080025B4 */
    .long   Default_Handler         /* [13] 0x080025B4 */
    .long   Default_Handler         /* [14] 0x080025B4 */
    .long   Default_Handler         /* [15] 0x080025B4 */
    .long   Default_Handler         /* [16] 0x080025B4 - UART0 would be here in app */
    .long   Default_Handler         /* [17] 0x080025B4 */
    .long   Default_Handler         /* [18] 0x080025B4 */
    .long   Default_Handler         /* [19] 0x080025B4 */
    .long   Default_Handler         /* [20] 0x080025B4 */
    .long   Default_Handler         /* [21] 0x080025B4 */
    .long   Default_Handler         /* [22] 0x080025B4 - tspend in app startup.S */
    .long   Default_Handler         /* [23] 0x080025B4 */
    .long   Default_Handler         /* [24] 0x080025B4 */
    .long   Default_Handler         /* [25] 0x080025B4 */
    .long   Default_Handler         /* [26] 0x080025B4 */
    .long   Default_Handler         /* [27] 0x080025B4 */
    .long   Default_Handler         /* [28] 0x080025B4 */
    .long   Default_Handler         /* [29] 0x080025B4 */
    .long   Default_Handler         /* [30] 0x080025B4 */
    .long   Default_Handler         /* [31] 0x080025B4 */
    .long   Default_Handler         /* [32] 0x080025B4 - External IRQs start (SDIO) */
    .long   Default_Handler         /* [33] 0x080025B4 - MAC */
    .long   Default_Handler         /* [34] 0x080025B4 - RF_CFG */
    .long   Default_Handler         /* [35] 0x080025B4 - SEC */
    .long   Default_Handler         /* [36] 0x080025B4 - DMA_CH0 */
    .long   Default_Handler         /* [37] 0x080025B4 - DMA_CH1 */
    .long   Default_Handler         /* [38] 0x080025B4 - DMA_CH2 */
    .long   Default_Handler         /* [39] 0x080025B4 - DMA_CH3 */
    .long   Default_Handler         /* [40] 0x080025B4 - DMA_CH4_7 */
    .long   Default_Handler         /* [41] 0x080025B4 - DMA_BURST */
    .long   Default_Handler         /* [42] 0x080025B4 - I2C */
    .long   Default_Handler         /* [43] 0x080025B4 - ADC */
    .long   Default_Handler         /* [44] 0x080025B4 - SPI_LS */
    .long   Default_Handler         /* [45] 0x080025B4 - SPI_HS */
    .long   Default_Handler         /* [46] 0x080025B4 - GPIOA */
    .long   Default_Handler         /* [47] 0x080025B4 - GPIOB */
    .long   Default_Handler         /* [48] 0x080025B4 - UART0 */
    .long   Default_Handler         /* [49] 0x080025B4 - UART1 */
    .long   Default_Handler         /* [50] 0x080025B4 - TOUCH */
    .long   Default_Handler         /* [51] 0x080025B4 - UART2_5 */
    .long   Default_Handler         /* [52] 0x080025B4 - BLE */
    .long   Default_Handler         /* [53] 0x080025B4 - BT */
    .long   Default_Handler         /* [54] 0x080025B4 - PWM */
    .long   Default_Handler         /* [55] 0x080025B4 - I2S */
    .long   Default_Handler         /* [56] 0x080025B4 - SDIO_HOST */
    .long   tspend_handler          /* [57] 0x08007148 - CORET/tspend */
    .long   Default_Handler         /* [58] 0x080025B4 - RSA */
    .long   Default_Handler         /* [59] 0x080025B4 - GPSEC */
    .long   Default_Handler         /* [60] 0x080025B4 - Flash */
    .long   Default_Handler         /* [61] 0x080025B4 - PMU */
    .long   Default_Handler         /* [62] 0x080025B4 - Timer0_5 */
    .long   Default_Handler         /* [63] 0x080025B4 - WDG */
    .size   __Vectors, . - __Vectors

/* ============================================================
 * Pre-Reset Hook (0x08002500)
 *
 * At offset 0x100 (just before Reset_Handler), there's a small
 * trampoline that loads and calls a function pointer from SRAM.
 * This allows a pre-initialized hook to run before Reset_Handler.
 * ============================================================ */
    .text
    .align 2
pre_reset_hook:
    /* 0x08002500 */
    lrw     r2, =pre_reset_hook_ptr     /* 0x200113E8 */
    ld.w    r3, (r2, 0x0)
    jsr     r3                          /* Call hook if pointer is set */

/* ============================================================
 * Reset_Handler (0x08002506)
 *
 * Entry point after reset. Initializes CPU state, copies .data
 * section from flash to SRAM, clears .bss, then calls:
 *   1. SystemInit()    - VBR, cache, interrupt controller
 *   2. board_init()    - Clock, UART
 *   3. main()          - Boot logic
 *
 * Cross-reference: platform/arch/xt804/bsp/startup.S
 * The pattern is nearly identical to the application startup.
 * ============================================================ */
    .globl  Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    /* 0x08002506: Set PSR - privileged mode, exceptions enabled */
    lrw     r0, 0xE0000200
    mtcr    r0, psr                     /* cr<0,0> = PSR */

    /* 0x0800250C: Set up stack pointer */
    lrw     r0, 0x200111D8              /* initial_sp in D-SRAM */
    mov     sp, r0                      /* r14 = sp */

    /* 0x08002510: Copy .data section (flash → SRAM) */
    lrw     r1, 0x08007604              /* __erodata (source in flash) */
    lrw     r2, 0x20010000              /* __data_start__ (dest in SRAM) */
    lrw     r3, 0x200101D8              /* __data_end__ */
    subu    r3, r2                      /* r3 = data section size */
    cmpnei  r3, 0
    bf      .L_data_done
.L_data_copy:
    ld.w    r0, (r1, 0x0)
    st.w    r0, (r2, 0x0)
    addi    r1, 4
    addi    r2, 4
    subi    r3, 4
    cmpnei  r3, 0
    bt      .L_data_copy
.L_data_done:

    /* 0x0800252A: Clear .bss section */
    lrw     r1, 0x200101D8              /* __bss_start__ */
    lrw     r2, 0x20011430              /* __bss_end__ */
    movi    r0, 0
    subu    r2, r1                      /* r2 = bss size */
    cmpnei  r2, 0
    bf      .L_bss_done
.L_bss_clear:
    st.w    r0, (r1, 0x0)
    addi    r1, 4
    subi    r2, 4
    cmpnei  r2, 0
    bt      .L_bss_clear
.L_bss_done:

    /* 0x08002540: Call initialization functions */
    bsr     SystemInit                  /* 0x080071B4 */
    bsr     board_init                  /* 0x0800712C */
    bsr     main                        /* 0x08007304 */

    /* 0x0800254C: Should never reach here */
__exit:
    br      __exit                      /* Infinite loop */
    .size   Reset_Handler, . - Reset_Handler

/* ============================================================
 * Default_Handler / Trap Handler (0x08002570)
 *
 * Handles all unhandled exceptions. Saves full register context
 * to a save area in SRAM, then calls trap_c (which loops forever).
 *
 * Cross-reference: platform/arch/xt804/bsp/vectors.S
 * ============================================================ */
    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    /* 0x08002570: Enable exceptions during handler */
    psrset  ee

    /* Save r13 (link register context) temporarily */
    subi    sp, 4
    st.w    r13, (sp, 0x0)
    addi    sp, 4

    /* Load save area pointer */
    lrw     r13, =trap_save_sp          /* 0x200113D8 */
    st.w    sp, (r13, 0x0)              /* Save original SP */

    lrw     sp, =trap_save_sp           /* 0x200113D8 */
    subi    sp, 72                      /* Allocate save frame */

    /* Save r0-r12 */
    stm     r0-r12, (sp)               /* 13 regs × 4 = 52 bytes */

    /* Restore and save original SP */
    lrw     r0, =trap_save_sp
    ld.w    r0, (r0, 0x0)
    st.w    r0, (sp, 0x38)             /* offset 56: original SP */

    /* Save r13 from temp location */
    subi    r0, 4
    ld.w    r13, (r0, 0x0)
    st.w    r13, (sp, 0x34)            /* offset 52: r13 */

    /* Save r15 (link register) */
    st.w    r15, (sp, 0x3C)            /* offset 60: r15/LR */

    /* Save EPC and EPSR */
    mfcr    r0, epc                    /* cr<2,0> = EPC */
    st.w    r0, (sp, 0x40)             /* offset 64: EPC */
    mfcr    r0, epsr                   /* cr<4,0> = EPSR */
    st.w    r0, (sp, 0x44)             /* offset 68: EPSR */

    /* Call trap_c with save frame pointer */
    mov     r0, sp
    bsr     trap_c                     /* 0x080071F0 - never returns */
    br      Default_Handler            /* Loop if it somehow returns */
    .size   Default_Handler, . - Default_Handler

/* ============================================================
 * tspend_handler (0x08007148) - ISR #57
 *
 * Timer/software pending interrupt handler.
 * Increments a counter at 0x200113E4.
 * Uses nie/nir for nested interrupt entry/return.
 * ============================================================ */
    .globl  tspend_handler
    .type   tspend_handler, %function
tspend_handler:
    nie                                /* Nested interrupt entry */
    ipush                              /* Push interrupt context */

    /* Save high registers and FPU state */
    subi    sp, 56
    stm     r18-r31, (sp)
    subi    sp, 32
    fstms   fr16-fr19, (sp)

    /* Read VIC status to acknowledge interrupt */
    lrw     r2, =tspend_counter        /* 0x200113E4 */
    lrw     r3, 0xE000E000             /* VIC_BASE */
    ld.w    r3, (r3, 0x10)             /* Read VIC status (acknowledge) */

    /* Increment counter */
    ld.w    r3, (r2, 0x0)
    addi    r3, 1
    st.w    r3, (r2, 0x0)

    /* Restore FPU and high registers */
    fldms   fr16-fr19, (sp)
    addi    sp, 32
    ldm     r18-r31, (sp)
    addi    sp, 56

    ipop                               /* Pop interrupt context */
    nir                                /* Nested interrupt return */
    .size   tspend_handler, . - tspend_handler
