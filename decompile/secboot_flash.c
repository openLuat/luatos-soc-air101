/**
 * secboot_flash.c - Decompiled flash operations
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/driver/wm_internal_flash.h, include/wm_regs.h
 *
 * Functions:
 *   flash_init()      - 0x08005338
 *   flash_read_page() - 0x080051E8
 *   flash_read()      - 0x080054F8
 *   flash_write()     - 0x0800553C
 *   flash_copy_data() - 0x0800582C
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
