/**
 * secboot_uart.c - Decompiled UART driver and boot detection
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/wm_regs.h, tools/xt804/wm_tool.c
 *
 * UART0 is used for:
 *   1. Console output (boot messages)
 *   2. Boot mode detection (checking for UART activity)
 *   3. Xmodem firmware download
 *
 * Functions:
 *   uart_init()       - 0x080058B8
 *   uart_rx_ready()   - 0x0800588C
 *   uart_getchar()    - 0x080058A0
 *   uart_putchar()    - 0x0800717C
 *   boot_uart_check() - 0x08007220
 *   puts()            - 0x080028F4
 */

#include "secboot_common.h"


/* ============================================================
 * uart_init (0x080058B8)
 *
 * Initialize UART0 with specified baud rate.
 * Calculates divisor from APB_CLK (40MHz).
 *
 * r0 = baud_rate (e.g., 115200)
 *
 * Baud rate register format:
 *   bits [15:0]  = integer divisor - 1
 *   bits [19:16] = fractional divisor (0-15)
 *
 * Formula: divisor = APB_CLK / (16 * baud_rate)
 *          frac = (APB_CLK - divisor * 16 * baud_rate) * 16 / (16 * baud_rate)
 *
 * Disassembly:
 *   80058b8: lsli r1, r0, 4           ; r1 = baud * 16
 *   80058ba: lrw  r3, 0x2625A00       ; r3 = 40000000 (APB_CLK)
 *   80058bc: divs r2, r3, r1          ; r2 = 40000000 / (baud*16) = int_div
 *   80058c0: mult r0, r2, r1          ; r0 = int_div * (baud*16)
 *   80058c4: subu r3, r0              ; r3 = remainder
 *   80058c6: lsli r0, r3, 4           ; r0 = remainder * 16
 *   80058c8: divs r0, r0, r1          ; r0 = frac_div
 *   80058cc: lrw  r3, 0x40010600      ; UART0_BASE
 *   80058ce: subi r2, 1               ; int_div -= 1
 *   80058d0: lsli r0, r0, 16          ; frac in upper bits
 *   80058d2: or   r0, r2              ; combine int + frac
 *   80058d4: movi r2, 195             ; 0xC3 = line control
 *   80058d6: st.w r0, (r3, 0x10)      ; Write BAUD_RATE
 *   80058d8: st.w r2, (r3, 0x00)      ; Write LINE_CTRL (8N1, FIFO enable)
 *   80058da: movi r2, 0
 *   80058dc: st.w r2, (r3, 0x04)      ; DMA_CTRL = 0
 *   80058de: st.w r2, (r3, 0x08)      ; FLOW_CTRL = 0
 *   80058e0: st.w r2, (r3, 0x0c)      ; INT_MASK = 0 (no interrupts)
 *   80058e2: jmp  r15
 * ============================================================ */
void uart_init(uint32_t baud_rate)
{
    volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;
    uint32_t baud16 = baud_rate << 4;       /* baud_rate * 16 */
    uint32_t int_div = APB_CLK / baud16;    /* Integer divisor */
    uint32_t remainder = APB_CLK - (int_div * baud16);
    uint32_t frac_div = (remainder << 4) / baud16; /* Fractional divisor */

    /* Write baud rate register: [19:16]=frac, [15:0]=int_div-1 */
    uart[UART_BAUD_RATE / 4] = (frac_div << 16) | (int_div - 1);

    /* Line control: 0xC3 = 8 data bits, no parity, 1 stop bit, FIFO enabled */
    uart[UART_LINE_CTRL / 4] = 0xC3;

    /* Disable DMA, flow control, and interrupts */
    uart[UART_DMA_CTRL / 4]  = 0;
    uart[UART_FLOW_CTRL / 4] = 0;
    uart[UART_INT_MASK / 4]  = 0;
}


/* ============================================================
 * uart_rx_ready (0x0800588C)
 *
 * Check if UART0 RX FIFO has data available.
 *
 * Returns: 1 if data available, 0 if empty
 *
 * Disassembly:
 *   800588c: lrw  r3, 0x40010600       ; UART0_BASE
 *   800588e: ld.w r3, (r3, 0x1c)       ; Read FIFO_STATUS
 *   8005890: andi r3, r3, 4032         ; Mask RX FIFO count (bits 11:6)
 *   8005894: cmpnei r3, 0
 *   8005896: mvc  r0                   ; r0 = (rx_count != 0) ? 1 : 0
 *   800589a: jmp  r15
 * ============================================================ */
int uart_rx_ready(void)
{
    volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;
    uint32_t fifo_status = uart[UART_FIFO_STATUS / 4];

    /* Check RX FIFO count (bits [11:6]) */
    return (fifo_status & UART_RX_FIFO_CNT_MASK) != 0;
}


/* ============================================================
 * uart_getchar (0x080058A0)
 *
 * Read one byte from UART0 RX. Blocks until data is available.
 *
 * Returns: received byte (0-255)
 *
 * Disassembly:
 *   80058a0: lrw  r2, 0x40010600       ; UART0_BASE
 *   80058a2: ld.w r3, (r2, 0x1c)       ; Read FIFO_STATUS
 *   80058a4: andi r3, r3, 4032         ; Mask RX count
 *   80058a8: bez  r3, 0x80058a2        ; Loop until data available
 *   80058ac: ld.w r0, (r2, 0x30)       ; Read RX_DATA
 *   80058ae: zextb r0, r0              ; Zero-extend to byte
 *   80058b0: jmp  r15
 * ============================================================ */
uint8_t uart_getchar(void)
{
    volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;

    /* Wait for RX FIFO to have data */
    while ((uart[UART_FIFO_STATUS / 4] & UART_RX_FIFO_CNT_MASK) == 0) {
        /* Spin */
    }

    /* Read and return one byte */
    return (uint8_t)(uart[UART_RX_DATA / 4] & 0xFF);
}


/* ============================================================
 * uart_putchar (0x0800717C)
 *
 * Write one byte to UART0 TX. Blocks until TX FIFO has space.
 * Converts '\n' (0x0A) to '\r\n' (0x0D, 0x0A).
 *
 * r0 = character to send
 *
 * Disassembly:
 *   800717c: cmpnei r0, 10            ; Is it '\n'?
 *   800717e: bf   handle_newline       ; Yes → send \r first
 *   8007180: lrw  r2, 0x40010600      ; UART0_BASE
 *   8007182: ld.w r3, (r2, 0x1c)      ; Read FIFO_STATUS
 *   8007184: andi r3, r3, 63          ; TX FIFO count (bits [5:0])
 *   8007188: bnez r3, 0x8007182       ; Wait until TX FIFO empty
 *   800718c: andi r0, r0, 255         ; Mask to byte
 *   8007190: st.w r0, (r2, 0x20)      ; Write TX_DATA
 *   8007192: mov  r0, r3              ; Return 0 (success)
 *   8007194: jmp  r15
 * handle_newline:
 *   8007196: lrw  r2, 0x40010600
 *   8007198: ld.w r3, (r2, 0x1c)      ; Wait for TX FIFO empty
 *   800719a: andi r3, r3, 63
 *   800719e: bnez r3, 0x8007198
 *   80071a2: movi r3, 13              ; '\r'
 *   80071a4: st.w r3, (r2, 0x20)      ; Send '\r'
 *   80071a6: br   0x8007180           ; Then send original '\n'
 * ============================================================ */
int uart_putchar(int ch)
{
    volatile uint32_t *uart = (volatile uint32_t *)UART0_BASE;

    if (ch == '\n') {
        /* Send carriage return first */
        while (uart[UART_FIFO_STATUS / 4] & UART_TX_FIFO_CNT_MASK) {
            /* Wait for TX FIFO to drain */
        }
        uart[UART_TX_DATA / 4] = '\r';
    }

    /* Wait for TX FIFO space */
    while (uart[UART_FIFO_STATUS / 4] & UART_TX_FIFO_CNT_MASK) {
        /* Wait */
    }

    /* Send character */
    uart[UART_TX_DATA / 4] = (uint8_t)ch;
    return 0;
}


/* ============================================================
 * boot_uart_check (0x08007220)
 *
 * Check if there's UART activity indicating bootloader mode.
 * Polls UART0 RX for approximately 196,606 iterations.
 * If 3 consecutive non-ESC (0x1B) bytes are received,
 * enters bootloader mode.
 *
 * Returns: 0 = no UART activity (normal boot)
 *          1 = UART bootloader mode detected
 *
 * The ESC character (0x1B = 27) resets the counter, allowing
 * the host tool to "wake up" the bootloader without triggering
 * download mode accidentally.
 *
 * Disassembly:
 *   8007220: push r4-r7, r15
 *   8007222: subi sp, 4
 *   8007224: movi r5, 0                ; consecutive_count = 0
 *   8007226: movih r4, 3               ; r4 = 0x30000
 *   800722a: st.w r5, (sp, 0)          ; poll_count = 0
 *   800722c: subi r4, 2                ; r4 = 0x2FFFE = 196606
 *   800722e: mov  r6, r5               ; r6 = 0
 *   8007230: movi r7, 2                ; threshold = 2 (need 3 bytes)
 * loop:
 *   8007232: ld.w r3, (sp, 0)          ; load poll_count
 *   8007234: cmplt r4, r3              ; poll_count > max?
 *   8007236: bt   timeout              ; Yes → no UART
 *   8007238: bsr  uart_rx_ready        ; Check RX FIFO
 *   800723c: bnez r0, got_byte         ; Data available
 *   8007240: ld.w r3, (sp, 0)          ; Increment poll_count
 *   8007242: addi r3, 1
 *   8007244: st.w r3, (sp, 0)
 *   ...continue polling...
 *   80071f0: bf   loop
 * timeout:
 *   800724c: movi r0, 0                ; Return 0 (no UART)
 *   800724e: addi sp, 4
 *   8007250: pop  r4-r7, r15
 * got_byte:
 *   8007252: bsr  uart_getchar         ; Read the byte
 *   8007256: cmpnei r0, 27            ; Is it ESC (0x1B)?
 *   8007258: bf   reset_count          ; Yes → reset counter
 *   800725a: movi r5, 0               ; reset consecutive
 *   800725c: br   loop
 * reset_count:
 *   800725e: addi r5, 1               ; consecutive_count++
 *   8007260: zextb r5, r5             ; Mask to byte
 *   8007262: cmphs r7, r5             ; count <= threshold?
 *   8007264: st.w r6, (sp, 0)          ; Reset poll_count
 *   8007266: bt   loop                 ; Keep polling
 *   ; Fell through: consecutive_count > 2 → bootloader mode!
 *   8007268: lrw  r3, 0x200113EC       ; boot_mode_flag
 *   800726a: movi r0, 1
 *   800726c: st.b r6, (r3, 0)          ; Clear flag byte
 *   800726e: addi sp, 4
 *   8007270: pop  r4-r7, r15          ; Return 1
 * ============================================================ */
int boot_uart_check(void)
{
    uint32_t poll_count = 0;
    uint32_t max_polls = 0x2FFFE;   /* ~196606 iterations */
    uint8_t consecutive = 0;
    uint8_t threshold = 2;          /* Need 3 non-ESC bytes (> 2) */

    while (poll_count <= max_polls) {
        if (uart_rx_ready()) {
            uint8_t ch = uart_getchar();

            if (ch == 0x1B) {
                /* ESC received - reset counter but keep polling */
                consecutive = 0;
            } else {
                consecutive++;
                if (consecutive > threshold) {
                    /* 3+ non-ESC bytes received → bootloader mode */
                    volatile uint8_t *boot_flag = (volatile uint8_t *)0x200113EC;
                    *boot_flag = 0;
                    return 1;
                }
            }
            poll_count = 0;  /* Reset poll counter on any byte */
        } else {
            poll_count++;
        }
    }

    return 0;   /* Timeout - no UART activity → normal boot */
}


/* ============================================================
 * puts (0x080028F4)
 *
 * Print a null-terminated string via uart_putchar.
 * (Simplified reconstruction - actual implementation uses
 *  the printf infrastructure)
 * ============================================================ */
void puts(const char *str)
{
    while (*str) {
        uart_putchar(*str++);
    }
}
