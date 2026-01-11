#include "ata.h"
#include "ports.h"

static void ata_wait_bsy() {
    while (port_byte_in(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
}

static void ata_wait_drq() {
    while (!(port_byte_in(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ));
}

int ata_read_sector(uint32_t lba, uint16_t* buffer) {
    ata_wait_bsy();

    port_byte_out(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    port_byte_out(ATA_PRIMARY_SEC_COUNT, 1);
    port_byte_out(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    port_byte_out(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    port_byte_out(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    port_byte_out(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++) {
        buffer[i] = port_word_in(ATA_PRIMARY_DATA);
    }

    return 0;
}
