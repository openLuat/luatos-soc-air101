/**
 * secboot_hw_init.c - Decompiled hardware initialization
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/wm_regs.h, platform/arch/xt804/bsp/startup.S
 *
 * Functions:
 *   SystemInit()  - 0x080071B4
 *   board_init()  - 0x0800712C
 *   trap_c()      - 0x080071F0
 *   lock_acquire()- 0x080071AC
 *   lock_release()- 0x080071B0
 */

#include <stdint.h>

/* ============================================================
 * Register Definitions (from include/wm_regs.h)
 * ============================================================ */

/* C-SKY Control Registers */
#define CR_PSR      0   /* cr<0,0> - Processor Status Register */
#define CR_VBR      1   /* cr<1,0> - Vector Base Register */
#define CR_EPSR     4   /* cr<4,0> - Exception PSR */
#define CR_EPC      2   /* cr<2,0> - Exception PC */
#define CR_HINT     31  /* cr<31,0> - Cache hint/control */

/* VIC (Vectored Interrupt Controller) - E000E000 */
#define VIC_BASE            0xE000E000
#define VIC_INT_EN_BASE     0xE000E100
#define VIC_INT_PEND_OFFSET 0x200   /* Offset from VIC_INT_EN_BASE */
#define VIC_INT_CLR_OFFSET  0x180   /* Offset from VIC_INT_EN_BASE */

/* Clock / PMU Registers */
#define CLK_BASE            0x40011400
#define CLK_CTRL_OFFSET     0x0C

/* SRAM addresses */
#define INITIAL_SP          0x200111D8
#define TRAP_SAVE_SP        0x200113D8
#define TSPEND_COUNTER      0x200113E4

/* ============================================================
 * SystemInit (0x080071B4)
 *
 * Initializes the CPU core:
 *   1. Set VBR (Vector Base Register) to 0x08002400
 *   2. Enable instruction cache
 *   3. Enable unaligned access
 *   4. Configure VIC interrupt controller
 *   5. Enable exceptions and interrupts (PSR.EE + PSR.IE)
 *
 * Disassembly:
 *   80071b4: lrw  r3, 0x8002400
 *   80071b6: mtcr r3, cr<1,0>         ; VBR = 0x08002400
 *   80071ba: mfcr r3, cr<31,0>        ; Read cache control
 *   80071be: ori  r3, r3, 16          ; Enable I-cache (bit 4)
 *   80071c2: mtcr r3, cr<31,0>        ; Write back
 *   80071c6: mfcr r3, cr<0,0>         ; Read PSR
 *   80071ca: ori  r3, r3, 512         ; Enable unaligned access (bit 9)
 *   80071ce: mtcr r3, cr<0,0>         ; Write back
 *   80071d2: lrw  r2, 0xe000e100      ; VIC_INT_EN base
 *   80071d4: movi r3, 0
 *   80071d6: st.w r3, (r2, 0x200)     ; Clear all pending interrupts
 *   80071da: subi r3, 1               ; r3 = 0xFFFFFFFF
 *   80071dc: st.w r3, (r2, 0x180)     ; Clear all interrupt enables
 *   80071e0: psrset ee, ie            ; Enable exceptions + interrupts
 *   80071e4: jmp  r15                 ; Return
 * ============================================================ */
void SystemInit(void)
{
    uint32_t reg;

    /* Set Vector Base Register to secboot's vector table */
    __set_VBR(0x08002400);

    /* Enable instruction cache (cr<31,0> bit 4) */
    reg = __get_HINT();
    reg |= (1 << 4);
    __set_HINT(reg);

    /* Enable unaligned memory access (PSR bit 9) */
    reg = __get_PSR();
    reg |= (1 << 9);
    __set_PSR(reg);

    /* Configure VIC: clear pending, clear enables */
    volatile uint32_t *vic_en = (volatile uint32_t *)VIC_INT_EN_BASE;
    vic_en[VIC_INT_PEND_OFFSET / 4] = 0;           /* Clear pending */
    vic_en[VIC_INT_CLR_OFFSET / 4] = 0xFFFFFFFF;   /* Clear all enables */

    /* Enable exceptions and interrupts in PSR */
    __psrset_ee_ie();
}


/* ============================================================
 * board_init (0x0800712C)
 *
 * Board-level initialization:
 *   1. Configure clock/PMU: clear bit 20 of CLK_CTRL
 *   2. Initialize UART0 at 115200 baud
 *
 * The baud rate value 0xC200 | (1<<16) = 49664 + 65536
 * is passed to uart_init, which expects the APB clock frequency.
 * 0xC200 = 49664; with bit 16 set = 115200 -- this encodes
 * the target baud rate for the uart_init calculation.
 *
 * Disassembly:
 *   800712c: push r15
 *   800712e: lrw  r2, 0x40011400       ; CLK_BASE
 *   8007130: movi r0, 49664            ; 0xC200
 *   8007134: bseti r0, 16             ; r0 = 0x1C200 (115200)
 *   8007136: ld.w r3, (r2, 0xc)       ; Read CLK_CTRL
 *   8007138: bclri r3, 20             ; Clear bit 20
 *   800713a: st.w r3, (r2, 0xc)       ; Write CLK_CTRL
 *   800713c: bsr  uart_init            ; 0x080058B8
 *   8007140: pop  r15                 ; Return
 * ============================================================ */
void board_init(void)
{
    volatile uint32_t *clk_base = (volatile uint32_t *)CLK_BASE;
    uint32_t clk_ctrl;

    /* Configure clock: clear bit 20 of CLK_CTRL register */
    clk_ctrl = clk_base[CLK_CTRL_OFFSET / 4];
    clk_ctrl &= ~(1 << 20);
    clk_base[CLK_CTRL_OFFSET / 4] = clk_ctrl;

    /* Initialize UART0 at 115200 baud */
    uart_init(115200);  /* 0x1C200 = 115200 */
}


/* ============================================================
 * trap_c (0x080071F0)
 *
 * C-level trap/exception handler. Called from Default_Handler
 * with a pointer to the saved register context.
 * Simply loops forever (unrecoverable exception).
 *
 * Disassembly:
 *   80071f0: br 0x80071f0              ; Infinite loop
 * ============================================================ */
void trap_c(void *context)
{
    (void)context;
    while (1) {
        /* Unrecoverable exception - halt */
    }
}


/* ============================================================
 * lock_acquire (0x080071AC) / lock_release (0x080071B0)
 *
 * Stub implementations - both just return 0.
 * In a full OS these would disable/enable interrupts.
 * In secboot, interrupts are minimally used, so these are no-ops.
 *
 * Disassembly:
 *   80071ac: movi r0, 0
 *   80071ae: jmp  r15
 *   80071b0: movi r0, 0
 *   80071b2: jmp  r15
 * ============================================================ */
int lock_acquire(uint32_t *lock)
{
    (void)lock;
    return 0;
}

int lock_release(uint32_t *lock)
{
    (void)lock;
    return 0;
}
