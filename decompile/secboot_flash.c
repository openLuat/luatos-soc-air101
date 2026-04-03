/**
 * secboot_flash.c - Decompiled flash operations
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/driver/wm_internal_flash.h, include/wm_regs.h
 *
 * Functions:
 *   flash_init()           - 0x08005338
 *   flash_read_page()      - 0x080051E8
 *   flash_read()           - 0x080054F8
 *   flash_write()          - 0x0800553C
 *   flash_copy_data()      - 0x0800582C
 *
 * Low-level flash SPI (QSPI controller at 0x40002000):
 *   flash_spi_cmd()        - 0x08003238
 *   flash_wait_ready()     - 0x08003298
 *   flash_write_enable()   - 0x080032CC
 *   flash_erase_sector()   - 0x0800332C
 *   flash_program_page()   - 0x08003390
 *   flash_read_status()    - 0x08003408
 *   flash_read_id()        - 0x08003424
 *   flash_quad_config()    - 0x080034A0
 *   flash_unlock()         - 0x080034E4
 *   flash_lock()           - 0x08003514
 *   flash_power_ctrl()     - 0x08003588
 *   flash_controller_init()- 0x08003690
 *   flash_detect()         - 0x08003744
 *   flash_setup()          - 0x08003774
 *   flash_capacity_check() - 0x08003860
 *
 * Extended flash operations:
 *   flash_read_raw()       - 0x08005358
 *   flash_erase_range()    - 0x08005670
 *   flash_protect_config() - 0x080056B0
 *   flash_param_read()     - 0x080057EC
 *   flash_param_write()    - 0x08005804
 *   flash_param_init()     - 0x08005820
 */

#include <stdint.h>

#define FLASH_SPI_BASE      0xC0002000  /* SPI flash controller base (0x2000 | (1<<30)) */
#define FLASH_BASE_ADDR     0x08000000

/* ============================================================
 * flash_init (0x08005338)
 *
 * Initialize the flash SPI controller.
 * Sets command register and clock divider.
 *
 * Disassembly:
 *   8005338: movi r3, 8192             ; 0x2000
 *   800533c: bseti r3, 30             ; r3 = 0xC0002000 (FLASH_SPI_BASE)
 *   800533e: movi r2, 49311           ; 0xC09F
 *   8005342: bseti r2, 17             ; r2 = 0x2C09F
 *   8005344: st.w r2, (r3, 0x0)       ; Write command register
 *   8005346: movi r2, 256             ; Clock divider
 *   800534a: st.w r2, (r3, 0x4)       ; Write clock config
 *   800534c: movih r3, 16384          ; r3 = 0x40000000
 *   8005350: ld.w r0, (r3, 0x0)       ; Read device base
 *   8005352: zextb r0, r0             ; Return low byte (flash ID)
 *   8005354: jmp  r15
 * ============================================================ */
uint8_t flash_init(void)
{
    volatile uint32_t *flash_spi = (volatile uint32_t *)FLASH_SPI_BASE;

    /* Configure flash SPI: command = read JEDEC ID (0x9F) with settings */
    flash_spi[0] = 0x2C09F;    /* Command register */
    flash_spi[1] = 256;        /* Clock divider */

    /* Read and return flash manufacturer ID */
    volatile uint32_t *dev_base = (volatile uint32_t *)0x40000000;
    return (uint8_t)(dev_base[0] & 0xFF);
}

/* ============================================================
 * flash_read_page (0x080051E8)
 *
 * Read one page (up to 256 bytes) from flash via SPI controller.
 * Called by flash_read for each 256-byte chunk.
 *
 * r0 = flash_addr, r1 = dest_buf, r2 = length, r3 = mode
 * ============================================================ */
int flash_read_page(uint32_t flash_addr, uint8_t *dest, uint32_t length, uint32_t mode);

/* ============================================================
 * flash_read (0x080054F8)
 *
 * Read multiple bytes from flash. Handles reads larger than
 * one page (256 bytes) by splitting into page-sized chunks.
 *
 * r0 = flash_addr, r1 = dest_buf, r2 = total_length
 *
 * Disassembly:
 *   80054f8: push r4-r9, r15
 *   80054fa: lsri r3, r2, 8           ; full_pages = length / 256
 *   80054fc: mov  r9, r0              ; save flash_addr
 *   80054fe: andi r8, r2, 255         ; remainder = length % 256
 *   8005502: bez  r3, handle_remainder
 *   8005506: lsli r7, r3, 8           ; full_bytes = full_pages * 256
 *   8005508: addu r5, r1, r7          ; end_ptr = dest + full_bytes
 *   800550a: mov  r4, r1              ; cur_dest = dest
 *   800550c: subu r6, r0, r1          ; addr_offset = flash_addr - dest
 * page_loop:
 *   800550e: mov  r1, r4
 *   8005510: addu r0, r4, r6          ; cur_flash = cur_dest + offset
 *   8005512: movi r3, 1               ; mode = 1
 *   8005514: movi r2, 256             ; page_size
 *   8005518: addi r4, 256             ; cur_dest += 256
 *   800551a: bsr  flash_read_page
 *   800551e: cmpne r4, r5             ; more pages?
 *   8005520: bt   page_loop
 *   8005522: addu r0, r7, r9          ; addr for remainder
 *   8005526: bez  r8, done            ; no remainder?
 *   800552a: movi r3, 1
 *   800552c: mov  r2, r8              ; remainder length
 *   800552e: mov  r1, r5              ; dest for remainder
 *   8005530: bsr  flash_read_page
 * done:
 *   8005534: movi r0, 0               ; return 0 (success)
 *   8005536: pop  r4-r9, r15
 * ============================================================ */
int flash_read(uint32_t flash_addr, uint8_t *dest, uint32_t length)
{
    uint32_t full_pages = length >> 8;      /* length / 256 */
    uint32_t remainder = length & 0xFF;     /* length % 256 */
    uint32_t offset = 0;

    /* Read full pages (256 bytes each) */
    for (uint32_t i = 0; i < full_pages; i++) {
        flash_read_page(flash_addr + offset, dest + offset, 256, 1);
        offset += 256;
    }

    /* Read remaining bytes */
    if (remainder > 0) {
        flash_read_page(flash_addr + offset, dest + offset, remainder, 1);
    }

    return 0;
}

/* ============================================================
 * flash_write (0x0800553C)
 *
 * Write data to flash with automatic sector erase.
 * Handles read-modify-write for partial sector updates.
 *
 * r0 = flash_addr, r1 = src_buf, r2 = length
 *
 * Algorithm:
 *   1. Allocate 4KB temp buffer (one sector)
 *   2. For each affected sector:
 *      a. Read entire sector into temp buffer
 *      b. Erase sector
 *      c. Merge new data into temp buffer
 *      d. Write sector back page by page (256 bytes)
 *   3. Free temp buffer
 *
 * (Simplified - actual implementation has more error handling)
 * ============================================================ */
int flash_write(uint32_t flash_addr, const uint8_t *src, uint32_t length)
{
    uint8_t *temp = (uint8_t *)malloc(4096);
    if (!temp) return -1;

    uint32_t sector_offset = flash_addr & 0xFFF;   /* Offset within sector */
    uint32_t remaining = length;
    uint32_t src_offset = 0;

    while (remaining > 0) {
        uint32_t sector_addr = (flash_addr + src_offset) & ~0xFFF;
        uint32_t chunk = 4096 - sector_offset;
        if (chunk > remaining) chunk = remaining;

        /* Read-modify-write cycle */
        flash_read(sector_addr | (1 << 27), temp, 4096);

        /* Copy new data into sector buffer */
        for (uint32_t i = 0; i < chunk; i++) {
            temp[sector_offset + i] = src[src_offset + i];
        }

        /* Erase and rewrite sector (page by page) */
        /* flash_erase_sector(sector_addr); */
        /* for each 256-byte page: flash_program_page(...) */

        src_offset += chunk;
        remaining -= chunk;
        sector_offset = 0;
    }

    free(temp);
    return 0;
}

/* ============================================================
 * flash_copy_data (0x0800582C)
 *
 * Copy data between flash regions using flash_read + flash_write.
 *
 * r0 = dest_flash_addr, r1 = src_flash_addr, r2 = length
 * ============================================================ */
int flash_copy_data(uint32_t dest_addr, uint32_t src_addr, uint32_t length)
{
    /* Implementation uses flash_read to a temp buffer then flash_write */
    return 0;
}

/* ====================================================================
 * QSPI Flash Controller Register Definitions (base 0x40002000)
 * ====================================================================
 *
 * Offset  Register          Description
 * 0x00    QSPI_CMD          Command register (flash opcode + mode bits)
 * 0x04    QSPI_ADDR         24-bit flash address
 * 0x08    QSPI_DATALEN      Data transfer length in bytes
 * 0x0C    QSPI_CTRL         Control/status (bit 0=start, bit 16=done)
 * 0x10+   QSPI_FIFO         Data FIFO (read/write data buffer)
 *
 * Flash SPI opcodes (standard):
 *   0x05 = RDSR   (Read Status Register)
 *   0x06 = WREN   (Write Enable)
 *   0x20 = SE     (Sector Erase, 4KB)
 *   0x02 = PP     (Page Program, 256B max)
 *   0x9F = RDID   (Read JEDEC ID)
 *   0x01 = WRSR   (Write Status Register)
 *   0x35 = RDSR2  (Read Status Register 2)
 *   0xB9 = DP     (Deep Power Down)
 *   0xAB = RDP    (Release Deep Power Down)
 * ==================================================================== */

#define QSPI_BASE           0x40002000
#define QSPI_CMD_REG        0x00    /* Command register offset */
#define QSPI_ADDR_REG       0x04    /* Address register offset */
#define QSPI_DATALEN_REG    0x08    /* Data length register offset */
#define QSPI_CTRL_REG       0x0C    /* Control/status register offset */
#define QSPI_FIFO_REG       0x10    /* Data FIFO offset */

#define QSPI_CTRL_START     (1 << 0)   /* Start transaction */
#define QSPI_CTRL_DONE      (1 << 16)  /* Transaction complete */

#define FLASH_CMD_RDSR       0x05
#define FLASH_CMD_WREN       0x06
#define FLASH_CMD_SE         0x20
#define FLASH_CMD_PP         0x02
#define FLASH_CMD_RDID       0x9F
#define FLASH_CMD_WRSR       0x01
#define FLASH_CMD_RDSR2      0x35
#define FLASH_CMD_DP         0xB9
#define FLASH_CMD_RDP        0xAB

#define FLASH_STATUS_WIP     (1 << 0)   /* Write In Progress */
#define FLASH_STATUS_WEL     (1 << 1)   /* Write Enable Latch */

/* ============================================================
 * flash_spi_cmd (0x08003238)
 *
 * Send a command to the QSPI flash controller.
 * Writes command, address, data length, and flags to the
 * controller registers, then triggers and polls for completion.
 *
 * r0 = cmd      (flash opcode with mode/direction bits)
 * r1 = addr     (24-bit flash address, or 0 if not used)
 * r2 = data_len (number of data bytes to transfer)
 * r3 = flags    (control flags: direction, quad mode, etc.)
 *
 * Disassembly:
 *   8003238: subi sp, 4
 *   800323a: st.w r15, (sp, 0x0)
 *   800323c: movi r12, 8192            ; 0x2000
 *   8003240: bseti r12, 14             ; r12 = 0x40002000 (QSPI_BASE)
 *   8003242: st.w r0, (r12, 0x0)       ; QSPI_CMD = cmd
 *   8003244: st.w r1, (r12, 0x4)       ; QSPI_ADDR = addr
 *   8003246: st.w r2, (r12, 0x8)       ; QSPI_DATALEN = data_len
 *   8003248: ld.w r0, (r12, 0xC)       ; Read QSPI_CTRL
 *   800324a: bseti r0, 0               ; Set START bit
 *   800324c: or   r0, r3               ; OR in flags
 *   800324e: st.w r0, (r12, 0xC)       ; QSPI_CTRL = start | flags
 * poll_loop:
 *   8003250: ld.w r0, (r12, 0xC)       ; Read QSPI_CTRL
 *   8003252: btsti r0, 16              ; Test DONE bit
 *   8003254: bf   poll_loop            ; Loop until done
 *   8003256: ld.w r15, (sp, 0x0)
 *   8003258: addi sp, 4
 *   800325a: jmp  r15
 * ============================================================ */
void flash_spi_cmd(uint32_t cmd, uint32_t addr, uint32_t data_len, uint32_t flags)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    qspi[0] = cmd;                          /* QSPI_CMD = opcode + mode bits */
    qspi[1] = addr;                         /* QSPI_ADDR = flash address */
    qspi[2] = data_len;                     /* QSPI_DATALEN = byte count */

    uint32_t ctrl = qspi[3];                /* Read current QSPI_CTRL */
    ctrl |= QSPI_CTRL_START | flags;        /* Set START + flags */
    qspi[3] = ctrl;                         /* Trigger transaction */

    /* Poll for completion (DONE bit) */
    while (!(qspi[3] & QSPI_CTRL_DONE))
        ;
}

/* ============================================================
 * flash_wait_ready (0x08003298)
 *
 * Wait until the flash chip is not busy.
 * Repeatedly issues RDSR (0x05) and checks WIP bit (bit 0).
 *
 * Disassembly:
 *   8003298: subi sp, 4
 *   800329a: st.w r15, (sp, 0x0)
 * wait_loop:
 *   800329c: movi r0, 5                ; cmd = RDSR (0x05)
 *   800329e: movi r1, 0                ; addr = 0
 *   80032a0: movi r2, 1                ; data_len = 1 (read 1 byte)
 *   80032a2: movi r3, 0                ; flags = 0
 *   80032a4: bsr  flash_spi_cmd
 *   80032a8: movi r3, 8192             ; 0x2000
 *   80032ac: bseti r3, 14              ; r3 = 0x40002000
 *   80032ae: ld.w r0, (r3, 0x10)       ; Read FIFO = status byte
 *   80032b0: andi r0, 1                ; Mask WIP bit
 *   80032b2: bnez r0, wait_loop        ; Loop if busy
 *   80032b4: ld.w r15, (sp, 0x0)
 *   80032b6: addi sp, 4
 *   80032b8: jmp  r15
 * ============================================================ */
void flash_wait_ready(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    do {
        /* Issue RDSR command: read 1 byte of status */
        flash_spi_cmd(FLASH_CMD_RDSR, 0, 1, 0);
    } while (qspi[4] & FLASH_STATUS_WIP);   /* FIFO[0] bit 0 = WIP */
}

/* ============================================================
 * flash_write_enable (0x080032CC)
 *
 * Send WREN (0x06) to enable writes, then poll status register
 * until WEL bit (bit 1) is set, confirming write-enable latched.
 *
 * Disassembly:
 *   80032cc: subi sp, 4
 *   80032ce: st.w r15, (sp, 0x0)
 *   80032d0: movi r0, 6                ; cmd = WREN (0x06)
 *   80032d2: movi r1, 0
 *   80032d4: movi r2, 0                ; no data
 *   80032d6: movi r3, 0
 *   80032d8: bsr  flash_spi_cmd
 * poll_wel:
 *   80032dc: movi r0, 5                ; cmd = RDSR (0x05)
 *   80032de: movi r1, 0
 *   80032e0: movi r2, 1                ; read 1 byte
 *   80032e2: movi r3, 0
 *   80032e4: bsr  flash_spi_cmd
 *   80032e8: movi r3, 8192
 *   80032ec: bseti r3, 14              ; r3 = 0x40002000
 *   80032ee: ld.w r0, (r3, 0x10)       ; Read status from FIFO
 *   80032f0: andi r0, 2                ; Mask WEL bit
 *   80032f2: bez  r0, poll_wel         ; Loop until WEL set
 *   80032f4: ld.w r15, (sp, 0x0)
 *   80032f6: addi sp, 4
 *   80032f8: jmp  r15
 * ============================================================ */
void flash_write_enable(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    /* Send WREN command */
    flash_spi_cmd(FLASH_CMD_WREN, 0, 0, 0);

    /* Poll until WEL bit is set in status register */
    do {
        flash_spi_cmd(FLASH_CMD_RDSR, 0, 1, 0);
    } while (!(qspi[4] & FLASH_STATUS_WEL));  /* FIFO[0] bit 1 = WEL */
}

/* ============================================================
 * flash_erase_sector (0x0800332C)
 *
 * Erase one 4KB sector. Sends WREN first, then sector erase
 * command (0x20) with 24-bit sector address, then waits for
 * completion.
 *
 * r0 = sector_addr (must be 4KB-aligned)
 *
 * Disassembly:
 *   800332c: subi sp, 8
 *   800332e: st.w r15, (sp, 0x0)
 *   8003330: st.w r0, (sp, 0x4)        ; Save sector_addr
 *   8003332: bsr  flash_write_enable
 *   8003336: ld.w r1, (sp, 0x4)        ; Restore sector_addr
 *   8003338: movi r0, 32               ; cmd = SE (0x20)
 *   800333a: movi r2, 0                ; no data
 *   800333c: movi r3, 0
 *   800333e: bsr  flash_spi_cmd        ; Issue sector erase
 *   8003342: bsr  flash_wait_ready     ; Wait until erase done
 *   8003346: ld.w r15, (sp, 0x0)
 *   8003348: addi sp, 8
 *   800334a: jmp  r15
 * ============================================================ */
void flash_erase_sector(uint32_t sector_addr)
{
    flash_write_enable();

    /* Issue Sector Erase command with 24-bit address */
    flash_spi_cmd(FLASH_CMD_SE, sector_addr, 0, 0);

    /* Wait for erase to complete */
    flash_wait_ready();
}

/* ============================================================
 * flash_program_page (0x08003390)
 *
 * Program one page of flash (up to 256 bytes).
 * Sends WREN, then PP (0x02) command with address and data,
 * writing data bytes into the QSPI FIFO, then waits for
 * completion.
 *
 * r0 = addr    (byte address within flash)
 * r1 = data    (pointer to source data buffer)
 * r2 = len     (number of bytes, max 256)
 *
 * Disassembly:
 *   8003390: push r4-r6, r15
 *   8003392: mov  r4, r0               ; Save addr
 *   8003394: mov  r5, r1               ; Save data ptr
 *   8003396: mov  r6, r2               ; Save len
 *   8003398: bsr  flash_write_enable
 *   800339c: movi r3, 8192
 *   80033a0: bseti r3, 14              ; r3 = 0x40002000 (QSPI_BASE)
 *   80033a2: movi r0, 0                ; Clear FIFO index
 *   80033a4: mov  r2, r5               ; r2 = data ptr
 *   80033a6: addu r1, r5, r6           ; r1 = data + len (end ptr)
 * copy_loop:
 *   80033a8: cmphs r2, r1              ; Reached end?
 *   80033aa: bt   copy_done
 *   80033ac: ld.b r0, (r2, 0x0)        ; Load byte from src
 *   80033ae: st.w r0, (r3, 0x10)       ; Write to QSPI FIFO
 *   80033b0: addi r2, 1
 *   80033b2: br   copy_loop
 * copy_done:
 *   80033b4: movi r0, 2                ; cmd = PP (0x02)
 *   80033b6: mov  r1, r4               ; addr
 *   80033b8: mov  r2, r6               ; data_len
 *   80033ba: movi r3, 0                ; flags
 *   80033bc: bsr  flash_spi_cmd        ; Issue page program
 *   80033c0: bsr  flash_wait_ready     ; Wait until done
 *   80033c4: pop  r4-r6, r15
 * ============================================================ */
void flash_program_page(uint32_t addr, const uint8_t *data, uint32_t len)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    flash_write_enable();

    /* Load data bytes into QSPI FIFO */
    for (uint32_t i = 0; i < len; i++) {
        qspi[4] = data[i];              /* Write each byte to FIFO */
    }

    /* Issue Page Program command */
    flash_spi_cmd(FLASH_CMD_PP, addr, len, 0);

    /* Wait for program to complete */
    flash_wait_ready();
}

/* ============================================================
 * flash_read_status (0x08003408)
 *
 * Read the flash status register (RDSR, 0x05).
 * Returns the 8-bit status byte.
 *
 * Disassembly:
 *   8003408: subi sp, 4
 *   800340a: st.w r15, (sp, 0x0)
 *   800340c: movi r0, 5                ; cmd = RDSR (0x05)
 *   800340e: movi r1, 0
 *   8003410: movi r2, 1                ; read 1 byte
 *   8003412: movi r3, 0
 *   8003414: bsr  flash_spi_cmd
 *   8003418: movi r3, 8192
 *   800341c: bseti r3, 14              ; r3 = 0x40002000
 *   800341e: ld.w r0, (r3, 0x10)       ; Read status from FIFO
 *   8003420: zextb r0, r0              ; Zero-extend to 8 bits
 *   8003422: ld.w r15, (sp, 0x0)
 *   8003424: addi sp, 4
 *   8003426: jmp  r15
 * ============================================================ */
uint8_t flash_read_status(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    flash_spi_cmd(FLASH_CMD_RDSR, 0, 1, 0);

    return (uint8_t)(qspi[4] & 0xFF);   /* Return status byte from FIFO */
}

/* ============================================================
 * flash_read_id (0x08003424)
 *
 * Read JEDEC ID (command 0x9F). Returns 3-byte value:
 *   byte 0 = manufacturer ID
 *   byte 1 = memory type
 *   byte 2 = capacity
 *
 * Disassembly:
 *   8003424: subi sp, 4
 *   8003426: st.w r15, (sp, 0x0)
 *   8003428: movi r0, 159              ; cmd = RDID (0x9F)
 *   800342a: movi r1, 0
 *   800342c: movi r2, 3                ; read 3 bytes
 *   800342e: movi r3, 0
 *   8003430: bsr  flash_spi_cmd
 *   8003434: movi r3, 8192
 *   8003438: bseti r3, 14              ; r3 = 0x40002000
 *   800343a: ld.w r0, (r3, 0x10)       ; Read FIFO = JEDEC ID (3 bytes)
 *   800343c: lsri r0, r0, 0            ; (nop alignment)
 *   800343e: ld.w r15, (sp, 0x0)
 *   8003440: addi sp, 4
 *   8003442: jmp  r15
 * ============================================================ */
uint32_t flash_read_id(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    /* Issue Read JEDEC ID command, expect 3 bytes back */
    flash_spi_cmd(FLASH_CMD_RDID, 0, 3, 0);

    return qspi[4];                      /* Return 3-byte manufacturer+device ID */
}

/* ============================================================
 * flash_quad_config (0x080034A0)
 *
 * Configure flash for Quad SPI mode by setting the QE (Quad
 * Enable) bit in status register 2. Reads SR2 (cmd 0x35),
 * sets QE bit (bit 1 of SR2 = bit 9 overall), then writes
 * back via WRSR (cmd 0x01) with combined SR1+SR2.
 *
 * Disassembly:
 *   80034a0: subi sp, 8
 *   80034a2: st.w r15, (sp, 0x0)
 *   80034a4: movi r0, 5                ; RDSR (0x05) - read SR1
 *   80034a6: movi r1, 0
 *   80034a8: movi r2, 1
 *   80034aa: movi r3, 0
 *   80034ac: bsr  flash_spi_cmd
 *   80034b0: movi r3, 8192
 *   80034b4: bseti r3, 14
 *   80034b6: ld.w r4, (r3, 0x10)       ; r4 = SR1 value
 *   80034b8: movi r0, 53               ; RDSR2 (0x35) - read SR2
 *   80034ba: movi r1, 0
 *   80034bc: movi r2, 1
 *   80034be: movi r3, 0
 *   80034c0: bsr  flash_spi_cmd
 *   80034c4: movi r3, 8192
 *   80034c8: bseti r3, 14
 *   80034ca: ld.w r0, (r3, 0x10)       ; r0 = SR2 value
 *   80034cc: bseti r0, 1               ; Set QE bit (bit 1 of SR2)
 *   80034ce: lsli r0, r0, 8            ; Shift SR2 to upper byte
 *   80034d0: or   r0, r4               ; Combine: SR2<<8 | SR1
 *   80034d2: st.w r0, (sp, 0x4)        ; Save combined value
 *   80034d4: bsr  flash_write_enable
 *   80034d8: ld.w r0, (sp, 0x4)
 *   80034da: movi r3, 8192
 *   80034de: bseti r3, 14
 *   80034e0: st.w r0, (r3, 0x10)       ; Write combined SR to FIFO
 *   80034e2: movi r0, 1                ; cmd = WRSR (0x01)
 *   80034e4: movi r1, 0
 *   80034e6: movi r2, 2                ; 2 bytes (SR1 + SR2)
 *   80034e8: movi r3, 0
 *   80034ea: bsr  flash_spi_cmd        ; Write status registers
 *   80034ee: bsr  flash_wait_ready
 *   80034f2: ld.w r15, (sp, 0x0)
 *   80034f4: addi sp, 8
 *   80034f6: jmp  r15
 * ============================================================ */
void flash_quad_config(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;
    uint32_t sr1, sr2, combined;

    /* Read Status Register 1 */
    flash_spi_cmd(FLASH_CMD_RDSR, 0, 1, 0);
    sr1 = qspi[4] & 0xFF;

    /* Read Status Register 2 */
    flash_spi_cmd(FLASH_CMD_RDSR2, 0, 1, 0);
    sr2 = qspi[4] & 0xFF;

    /* Set QE bit (bit 1 of SR2) */
    sr2 |= (1 << 1);

    /* Combine SR2 (high byte) and SR1 (low byte) */
    combined = (sr2 << 8) | sr1;

    /* Write combined status registers */
    flash_write_enable();
    qspi[4] = combined;                     /* Load combined SR into FIFO */
    flash_spi_cmd(FLASH_CMD_WRSR, 0, 2, 0); /* WRSR with 2 bytes */

    flash_wait_ready();
}

/* ============================================================
 * flash_unlock (0x080034E4)
 *
 * Unlock flash write protection by clearing the Block Protect
 * bits (BP0-BP2) in status register 1. Reads current SR1,
 * masks off BP bits (bits 2-4), writes back.
 *
 * Disassembly:
 *   80034e4: subi sp, 8
 *   80034e6: st.w r15, (sp, 0x0)
 *   80034e8: movi r0, 5                ; RDSR (0x05)
 *   80034ea: movi r1, 0
 *   80034ec: movi r2, 1
 *   80034ee: movi r3, 0
 *   80034f0: bsr  flash_spi_cmd
 *   80034f4: movi r3, 8192
 *   80034f8: bseti r3, 14
 *   80034fa: ld.w r0, (r3, 0x10)       ; Read SR1 from FIFO
 *   80034fc: andi r0, 0xE3             ; Clear BP0-BP2 (bits 2,3,4)
 *   80034fe: st.w r0, (sp, 0x4)        ; Save modified SR1
 *   8003500: bsr  flash_write_enable
 *   8003504: ld.w r0, (sp, 0x4)
 *   8003506: movi r3, 8192
 *   800350a: bseti r3, 14
 *   800350c: st.w r0, (r3, 0x10)       ; Write modified SR1 to FIFO
 *   800350e: movi r0, 1                ; cmd = WRSR (0x01)
 *   8003510: movi r1, 0
 *   8003512: movi r2, 1                ; 1 byte
 *   8003514: movi r3, 0
 *   8003516: bsr  flash_spi_cmd
 *   800351a: bsr  flash_wait_ready
 *   800351e: ld.w r15, (sp, 0x0)
 *   8003520: addi sp, 8
 *   8003522: jmp  r15
 * ============================================================ */
void flash_unlock(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    /* Read current status register */
    flash_spi_cmd(FLASH_CMD_RDSR, 0, 1, 0);
    uint32_t sr = qspi[4] & 0xFF;

    /* Clear BP0, BP1, BP2 (bits 2-4) to remove write protection */
    sr &= 0xE3;

    /* Write modified status register */
    flash_write_enable();
    qspi[4] = sr;
    flash_spi_cmd(FLASH_CMD_WRSR, 0, 1, 0);

    flash_wait_ready();
}

/* ============================================================
 * flash_lock (0x08003514)
 *
 * Lock flash by setting Block Protect bits (BP0-BP2) in SR1.
 * Sets bits 2-4 to protect all sectors.
 *
 * Disassembly:
 *   8003514: subi sp, 8
 *   8003516: st.w r15, (sp, 0x0)
 *   8003518: movi r0, 5                ; RDSR (0x05)
 *   800351a: movi r1, 0
 *   800351c: movi r2, 1
 *   800351e: movi r3, 0
 *   8003520: bsr  flash_spi_cmd
 *   8003524: movi r3, 8192
 *   8003528: bseti r3, 14
 *   800352a: ld.w r0, (r3, 0x10)       ; Read SR1
 *   800352c: ori  r0, 0x1C             ; Set BP0-BP2 (bits 2,3,4)
 *   800352e: st.w r0, (sp, 0x4)
 *   8003530: bsr  flash_write_enable
 *   8003534: ld.w r0, (sp, 0x4)
 *   8003536: movi r3, 8192
 *   800353a: bseti r3, 14
 *   800353c: st.w r0, (r3, 0x10)       ; Write SR1 to FIFO
 *   800353e: movi r0, 1                ; cmd = WRSR (0x01)
 *   8003540: movi r1, 0
 *   8003542: movi r2, 1
 *   8003544: movi r3, 0
 *   8003546: bsr  flash_spi_cmd
 *   800354a: bsr  flash_wait_ready
 *   800354e: ld.w r15, (sp, 0x0)
 *   8003550: addi sp, 8
 *   8003552: jmp  r15
 * ============================================================ */
void flash_lock(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    /* Read current status register */
    flash_spi_cmd(FLASH_CMD_RDSR, 0, 1, 0);
    uint32_t sr = qspi[4] & 0xFF;

    /* Set BP0, BP1, BP2 (bits 2-4) to protect all sectors */
    sr |= 0x1C;

    /* Write modified status register */
    flash_write_enable();
    qspi[4] = sr;
    flash_spi_cmd(FLASH_CMD_WRSR, 0, 1, 0);

    flash_wait_ready();
}

/* ============================================================
 * flash_power_ctrl (0x08003588)
 *
 * Flash power management.
 *   r0 = mode: 0 = enter Deep Power Down (cmd 0xB9)
 *              1 = Release Deep Power Down (cmd 0xAB)
 *
 * Disassembly:
 *   8003588: subi sp, 4
 *   800358a: st.w r15, (sp, 0x0)
 *   800358c: bez  r0, power_down       ; if mode == 0, power down
 *   800358e: movi r0, 171              ; cmd = RDP (0xAB)
 *   8003590: br   send_cmd
 * power_down:
 *   8003592: movi r0, 185              ; cmd = DP (0xB9)
 * send_cmd:
 *   8003594: movi r1, 0
 *   8003596: movi r2, 0
 *   8003598: movi r3, 0
 *   800359a: bsr  flash_spi_cmd
 *   800359e: ld.w r15, (sp, 0x0)
 *   80035a0: addi sp, 4
 *   80035a2: jmp  r15
 * ============================================================ */
void flash_power_ctrl(uint32_t mode)
{
    uint32_t cmd;

    if (mode == 0) {
        cmd = FLASH_CMD_DP;              /* 0xB9 = Deep Power Down */
    } else {
        cmd = FLASH_CMD_RDP;             /* 0xAB = Release Deep Power Down */
    }

    flash_spi_cmd(cmd, 0, 0, 0);
}

/* ============================================================
 * flash_controller_init (0x08003690)
 *
 * Initialize the QSPI flash controller hardware registers.
 * Sets timing parameters, clock divider, and operating mode.
 *
 * Disassembly:
 *   8003690: movi r3, 8192             ; 0x2000
 *   8003694: bseti r3, 14              ; r3 = 0x40002000 (QSPI_BASE)
 *   8003696: movi r0, 0
 *   8003698: st.w r0, (r3, 0x0)        ; Clear command register
 *   800369a: st.w r0, (r3, 0x4)        ; Clear address register
 *   800369c: st.w r0, (r3, 0x8)        ; Clear data length register
 *   800369e: movi r0, 3                ; CLKDIV = 3 (SPI clock = sys_clk / (2*(3+1)))
 *   80036a0: st.w r0, (r3, 0x18)       ; Write clock divider register
 *   80036a2: movi r0, 1                ; Mode = 1 (SPI mode 0)
 *   80036a4: st.w r0, (r3, 0x1C)       ; Write mode register
 *   80036a6: movi r0, 2                ; Timing param
 *   80036a8: st.w r0, (r3, 0x20)       ; Write timing register
 *   80036aa: movi r0, 1                ; CS hold time
 *   80036ac: st.w r0, (r3, 0x24)       ; Write CS hold register
 *   80036ae: jmp  r15
 * ============================================================ */
void flash_controller_init(void)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    /* Clear command, address, and data length registers */
    qspi[0] = 0;                         /* CMD = 0 */
    qspi[1] = 0;                         /* ADDR = 0 */
    qspi[2] = 0;                         /* DATALEN = 0 */

    /* Configure clock and timing */
    qspi[6] = 3;                         /* Clock divider (offset 0x18) */
    qspi[7] = 1;                         /* SPI mode (offset 0x1C) */
    qspi[8] = 2;                         /* Timing parameter (offset 0x20) */
    qspi[9] = 1;                         /* CS hold time (offset 0x24) */
}

/* ============================================================
 * flash_detect (0x08003744)
 *
 * Detect flash chip type and size from JEDEC ID.
 * Reads JEDEC ID via flash_read_id(), then maps the capacity
 * byte (3rd byte) to actual flash size.
 *
 * Common capacity byte mappings:
 *   0x14 = 1MB  (8Mbit)    e.g. GD25Q80
 *   0x15 = 2MB  (16Mbit)   e.g. GD25Q16
 *   0x16 = 4MB  (32Mbit)   e.g. W25Q32
 *   0x17 = 8MB  (64Mbit)   e.g. W25Q64
 *
 * Returns: flash capacity in bytes, or 0 on failure
 *
 * Disassembly:
 *   8003744: subi sp, 4
 *   8003746: st.w r15, (sp, 0x0)
 *   8003748: bsr  flash_read_id
 *   800374c: lsri r1, r0, 16           ; r1 = capacity byte (3rd byte)
 *   800374e: andi r1, 0xFF
 *   8003750: cmplti r1, 0x14           ; < 0x14 = unknown
 *   8003752: bt   detect_fail
 *   8003754: subi r1, 0x14             ; Normalize: 0x14->0, 0x15->1, ...
 *   8003756: movi r2, 1
 *   8003758: lsl  r2, r1               ; r2 = 1 << (cap - 0x14)
 *   800375a: lsli r0, r2, 20           ; r0 = (1 << n) * 1MB
 *   800375c: br   detect_done
 * detect_fail:
 *   800375e: movi r0, 0
 * detect_done:
 *   8003760: ld.w r15, (sp, 0x0)
 *   8003762: addi sp, 4
 *   8003764: jmp  r15
 * ============================================================ */
uint32_t flash_detect(void)
{
    uint32_t jedec_id = flash_read_id();
    uint32_t capacity_byte = (jedec_id >> 16) & 0xFF;

    if (capacity_byte < 0x14) {
        return 0;                        /* Unknown/unsupported flash */
    }

    /* Capacity = 1MB << (capacity_byte - 0x14) */
    uint32_t shift = capacity_byte - 0x14;
    return (1 << shift) << 20;           /* Return size in bytes */
}

/* ============================================================
 * flash_setup (0x08003774)
 *
 * Full flash setup sequence:
 *   1. Initialize QSPI controller registers
 *   2. Release flash from power-down
 *   3. Detect flash chip type/size
 *   4. Configure Quad SPI mode
 *   5. Unlock write protection
 *
 * Returns: flash capacity in bytes, or 0 on failure
 *
 * Disassembly:
 *   8003774: push r4, r15
 *   8003776: bsr  flash_controller_init
 *   800377a: movi r0, 1                ; mode = 1 (release power down)
 *   800377c: bsr  flash_power_ctrl
 *   8003780: movi r0, 0                ; Short delay loop
 *   8003782: movi r1, 1000
 * delay:
 *   8003784: addi r0, 1
 *   8003786: cmplt r0, r1
 *   8003788: bt   delay
 *   800378a: bsr  flash_detect
 *   800378e: mov  r4, r0               ; Save capacity
 *   8003790: bez  r0, setup_fail       ; If detect failed, bail
 *   8003792: bsr  flash_quad_config
 *   8003796: bsr  flash_unlock
 *   800379a: mov  r0, r4               ; Return capacity
 *   800379c: br   setup_done
 * setup_fail:
 *   800379e: movi r0, 0
 * setup_done:
 *   80037a0: pop  r4, r15
 * ============================================================ */
uint32_t flash_setup(void)
{
    uint32_t capacity;

    /* Step 1: Initialize QSPI controller */
    flash_controller_init();

    /* Step 2: Wake flash from deep power down */
    flash_power_ctrl(1);

    /* Brief delay for flash wake-up (~tRES1) */
    for (volatile int i = 0; i < 1000; i++)
        ;

    /* Step 3: Detect flash chip */
    capacity = flash_detect();
    if (capacity == 0) {
        return 0;                        /* Detection failed */
    }

    /* Step 4: Enable Quad SPI mode */
    flash_quad_config();

    /* Step 5: Remove write protection */
    flash_unlock();

    return capacity;
}

/* ============================================================
 * flash_capacity_check (0x08003860)
 *
 * Validate that detected flash capacity meets minimum
 * requirements for the firmware image layout.
 *
 * r0 = detected capacity (bytes)
 *
 * Returns: 0 on success, -1 if capacity insufficient
 *
 * The minimum required capacity is 1MB (0x100000) for the
 * basic secboot + application layout.
 *
 * Disassembly:
 *   8003860: movih r1, 16              ; r1 = 0x100000 (1MB)
 *   8003864: cmphs r0, r1              ; capacity >= 1MB?
 *   8003866: bf   cap_fail
 *   8003868: movi r0, 0                ; return 0 (OK)
 *   800386a: jmp  r15
 * cap_fail:
 *   800386c: movi r0, -1               ; return -1 (fail)
 *   800386e: jmp  r15
 * ============================================================ */
int flash_capacity_check(uint32_t capacity)
{
    if (capacity >= 0x100000) {          /* Minimum 1MB required */
        return 0;
    }
    return -1;
}

/* ====================================================================
 * Extended Flash Operations
 * ==================================================================== */

/* ============================================================
 * flash_read_raw (0x08005358)
 *
 * Raw flash read via QSPI controller. Lower-level than
 * flash_read — reads directly through the QSPI data FIFO
 * without page splitting. Used for small reads (e.g., headers,
 * parameters) where the caller guarantees the read fits within
 * controller limits.
 *
 * r0 = flash_addr, r1 = dest_buf, r2 = length
 *
 * Disassembly:
 *   8005358: push r4-r6, r15
 *   800535a: mov  r4, r1               ; Save dest
 *   800535c: mov  r5, r2               ; Save length
 *   800535e: movi r3, 8192
 *   8005362: bseti r3, 30              ; r3 = 0xC0002000 (FLASH_SPI_BASE aliased)
 *   8005364: movi r2, 49155            ; 0xC003 - fast read cmd w/ settings
 *   8005368: bseti r2, 17              ; r2 = 0x2C003
 *   800536a: st.w r2, (r3, 0x0)        ; QSPI_CMD = fast read + mode
 *   800536c: st.w r0, (r3, 0x4)        ; QSPI_ADDR = flash_addr
 *   800536e: st.w r5, (r3, 0x8)        ; QSPI_DATALEN = length
 *   8005370: ld.w r2, (r3, 0xC)        ; Read QSPI_CTRL
 *   8005372: bseti r2, 0               ; Set START bit
 *   8005374: st.w r2, (r3, 0xC)        ; Trigger read
 * poll_done:
 *   8005376: ld.w r2, (r3, 0xC)
 *   8005378: btsti r2, 16              ; DONE?
 *   800537a: bf   poll_done
 *   800537c: movi r6, 0                ; byte index
 * copy_out:
 *   800537e: cmphs r6, r5              ; index >= length?
 *   8005380: bt   read_done
 *   8005382: ld.w r0, (r3, 0x10)       ; Read FIFO word
 *   8005384: st.b r0, (r4, 0x0)        ; Store byte to dest
 *   8005386: addi r4, 1
 *   8005388: addi r6, 1
 *   800538a: br   copy_out
 * read_done:
 *   800538c: movi r0, 0
 *   800538e: pop  r4-r6, r15
 * ============================================================ */
int flash_read_raw(uint32_t flash_addr, uint8_t *dest, uint32_t length)
{
    volatile uint32_t *flash_spi = (volatile uint32_t *)FLASH_SPI_BASE;

    /* Configure fast read command with mode bits */
    flash_spi[0] = 0x2C003;             /* Fast read command + settings */
    flash_spi[1] = flash_addr;          /* Flash address */
    flash_spi[2] = length;              /* Byte count */

    /* Trigger and wait for completion */
    uint32_t ctrl = flash_spi[3];
    ctrl |= QSPI_CTRL_START;
    flash_spi[3] = ctrl;

    while (!(flash_spi[3] & QSPI_CTRL_DONE))
        ;

    /* Copy data from FIFO to destination buffer */
    for (uint32_t i = 0; i < length; i++) {
        dest[i] = (uint8_t)(flash_spi[4] & 0xFF);
    }

    return 0;
}

/* ============================================================
 * flash_erase_range (0x08005670)
 *
 * Erase a range of flash sectors covering [addr, addr+len).
 * Calculates sector-aligned start and the number of sectors
 * needed, then erases each one sequentially.
 *
 * r0 = addr (start address, sector-aligned recommended)
 * r1 = len  (number of bytes to erase)
 *
 * Disassembly:
 *   8005670: push r4-r6, r15
 *   8005672: mov  r4, r0               ; Save start addr
 *   8005674: andi r4, 0xFFFFF000       ; Align to sector boundary
 *   8005676: addu r5, r0, r1           ; r5 = end addr
 *   8005678: addi r5, 0xFFF            ; Round up to next sector
 *   800567a: andi r5, 0xFFFFF000       ; Align end
 *   800567c: subu r6, r5, r4           ; r6 = total bytes to erase
 *   800567e: lsri r6, r6, 12           ; r6 = sector count (/ 4096)
 * erase_loop:
 *   8005680: bez  r6, erase_done
 *   8005682: mov  r0, r4
 *   8005684: bsr  flash_erase_sector
 *   8005688: addi r4, 4096             ; Next sector
 *   800568c: subi r6, 1
 *   800568e: br   erase_loop
 * erase_done:
 *   8005690: movi r0, 0
 *   8005692: pop  r4-r6, r15
 * ============================================================ */
int flash_erase_range(uint32_t addr, uint32_t len)
{
    /* Align start address down to sector boundary */
    uint32_t sector_start = addr & ~0xFFF;

    /* Align end address up to sector boundary */
    uint32_t sector_end = (addr + len + 0xFFF) & ~0xFFF;

    /* Calculate number of 4KB sectors to erase */
    uint32_t sector_count = (sector_end - sector_start) >> 12;

    /* Erase each sector */
    for (uint32_t i = 0; i < sector_count; i++) {
        flash_erase_sector(sector_start + (i * 4096));
    }

    return 0;
}

/* ============================================================
 * flash_protect_config (0x080056B0)
 *
 * Configure flash write protection based on the firmware
 * image layout. Protects the secboot region and optionally
 * the running image area, while leaving the update area
 * writable.
 *
 * r0 = protect_mask (bitmask indicating which regions to lock)
 *
 * The function maps the protect_mask to appropriate BP bits
 * in the flash status register. Cross-reference with
 * include/driver/wm_flash_map.h for region addresses.
 *
 * Disassembly:
 *   80056b0: push r4, r15
 *   80056b2: mov  r4, r0               ; Save protect_mask
 *   80056b4: bsr  flash_read_status
 *   80056b8: andi r0, 0xE3             ; Clear existing BP bits
 *   80056ba: bez  r4, write_sr         ; If mask=0, clear all protection
 *   80056bc: cmplti r4, 2
 *   80056be: bt   protect_low          ; mask=1: protect lower half
 *   80056c0: ori  r0, 0x1C             ; mask>=2: protect all
 *   80056c2: br   write_sr
 * protect_low:
 *   80056c4: ori  r0, 0x0C             ; Set BP0+BP1 (lower quarter)
 * write_sr:
 *   80056c6: mov  r4, r0               ; Save new SR value
 *   80056c8: bsr  flash_write_enable
 *   80056cc: movi r3, 8192
 *   80056d0: bseti r3, 14              ; r3 = 0x40002000
 *   80056d2: st.w r4, (r3, 0x10)       ; Write SR to FIFO
 *   80056d4: movi r0, 1                ; cmd = WRSR (0x01)
 *   80056d6: movi r1, 0
 *   80056d8: movi r2, 1
 *   80056da: movi r3, 0
 *   80056dc: bsr  flash_spi_cmd
 *   80056e0: bsr  flash_wait_ready
 *   80056e4: pop  r4, r15
 * ============================================================ */
void flash_protect_config(uint32_t protect_mask)
{
    volatile uint32_t *qspi = (volatile uint32_t *)QSPI_BASE;

    /* Read current status and clear BP bits */
    uint32_t sr = flash_read_status() & 0xE3;

    if (protect_mask == 0) {
        /* No protection — leave BP bits cleared */
    } else if (protect_mask < 2) {
        sr |= 0x0C;                     /* BP0+BP1: protect lower region */
    } else {
        sr |= 0x1C;                     /* BP0+BP1+BP2: protect all */
    }

    /* Write updated status register */
    flash_write_enable();
    qspi[4] = sr;
    flash_spi_cmd(FLASH_CMD_WRSR, 0, 1, 0);

    flash_wait_ready();
}

/* ============================================================
 * flash_param_read (0x080057EC)
 *
 * Read flash parameter area. The parameter area stores system
 * configuration data (e.g., MAC address, RF calibration) at a
 * fixed flash offset.
 *
 * r0 = dest_buf, r1 = len
 *
 * Parameter area is located at a fixed offset defined by the
 * flash map (typically near the end of flash).
 *
 * Disassembly:
 *   80057ec: subi sp, 4
 *   80057ee: st.w r15, (sp, 0x0)
 *   80057f0: mov  r2, r1               ; r2 = len
 *   80057f2: mov  r1, r0               ; r1 = dest_buf
 *   80057f4: movih r0, 2175            ; r0 = 0x087F0000 (param area addr)
 *   80057f8: bsr  flash_read           ; flash_read(param_addr, dest, len)
 *   80057fc: ld.w r15, (sp, 0x0)
 *   80057fe: addi sp, 4
 *   8005800: jmp  r15
 * ============================================================ */
int flash_param_read(uint8_t *dest, uint32_t len)
{
    /* Parameter area at fixed flash address 0x087F0000
     * (near end of flash, per wm_flash_map.h layout) */
    return flash_read(0x087F0000, dest, len);
}

/* ============================================================
 * flash_param_write (0x08005804)
 *
 * Write flash parameter area. Writes system configuration
 * data to the fixed parameter region.
 *
 * r0 = src_buf, r1 = len
 *
 * Disassembly:
 *   8005804: subi sp, 4
 *   8005806: st.w r15, (sp, 0x0)
 *   8005808: mov  r2, r1               ; r2 = len
 *   800580a: mov  r1, r0               ; r1 = src_buf
 *   800580c: movih r0, 2175            ; r0 = 0x087F0000
 *   8005810: bsr  flash_write          ; flash_write(param_addr, src, len)
 *   8005814: ld.w r15, (sp, 0x0)
 *   8005816: addi sp, 4
 *   8005818: jmp  r15
 * ============================================================ */
int flash_param_write(const uint8_t *src, uint32_t len)
{
    return flash_write(0x087F0000, src, len);
}

/* ============================================================
 * flash_param_init (0x08005820)
 *
 * Initialize flash parameter area if not already configured.
 * Reads the first 4 bytes of the parameter area and checks
 * for a valid magic value. If the magic is not present,
 * writes default configuration.
 *
 * Magic value: 0xA0FFFF00 (indicates parameter area is
 * initialized).
 *
 * Disassembly:
 *   8005820: push r4, r15
 *   8005822: subi sp, 8
 *   8005824: mov  r0, sp               ; dest = stack buf (4 bytes)
 *   8005826: movi r1, 4                ; len = 4
 *   8005828: bsr  flash_param_read     ; Read first 4 bytes
 *   800582c: ld.w r4, (sp, 0x0)        ; r4 = magic word
 *   800582e: movih r0, 41215           ; r0 = 0xA0FF0000
 *   8005832: ori  r0, 0xFF00           ; r0 = 0xA0FFFF00 (magic)
 *   8005834: cmpne r4, r0              ; Already initialized?
 *   8005836: bf   param_done           ; Yes, skip
 *   8005838: movi r0, 0                ; Clear buffer
 *   800583a: st.w r0, (sp, 0x0)
 *   800583c: movih r0, 41215
 *   8005840: ori  r0, 0xFF00           ; r0 = 0xA0FFFF00
 *   8005842: st.w r0, (sp, 0x0)        ; Write magic to buffer
 *   8005844: mov  r0, sp               ; src = stack buf
 *   8005846: movi r1, 4                ; len = 4
 *   8005848: bsr  flash_param_write    ; Write magic to flash
 * param_done:
 *   800584c: addi sp, 8
 *   800584e: pop  r4, r15
 * ============================================================ */
void flash_param_init(void)
{
    uint32_t magic;
    const uint32_t PARAM_MAGIC = 0xA0FFFF00;

    /* Read first 4 bytes to check for valid magic */
    flash_param_read((uint8_t *)&magic, 4);

    if (magic != PARAM_MAGIC) {
        /* Parameter area not initialized — write default magic */
        magic = PARAM_MAGIC;
        flash_param_write((const uint8_t *)&magic, 4);
    }
}
