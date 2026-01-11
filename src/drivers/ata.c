#include "ata.h"
#include "ports.h"
#include "serial.h"

static uint8_t ata_get_status() {
    return port_byte_in(ATA_PRIMARY_STATUS);
}

static void ata_io_wait() {
    port_byte_in(ATA_PRIMARY_STATUS);
    port_byte_in(ATA_PRIMARY_STATUS);
    port_byte_in(ATA_PRIMARY_STATUS);
    port_byte_in(ATA_PRIMARY_STATUS);
}

static int ata_wait_for(uint8_t mask, uint8_t value, int timeout) {
    while (timeout--) {
        uint8_t status = ata_get_status();
        if ((status & mask) == value) return 0;
        if (status & ATA_STATUS_ERR) return -1;
        ata_io_wait();
    }
    return -2; // Timeout
}

static void ata_wait_bsy() {
    while (ata_get_status() & ATA_STATUS_BSY);
}

int ata_identify() {
    ata_wait_bsy();
    port_byte_out(ATA_PRIMARY_DRIVE_SEL, 0xA0); // Master
    ata_io_wait();
    port_byte_out(ATA_PRIMARY_SEC_COUNT, 0);
    port_byte_out(ATA_PRIMARY_LBA_LOW, 0);
    port_byte_out(ATA_PRIMARY_LBA_MID, 0);
    port_byte_out(ATA_PRIMARY_LBA_HIGH, 0);
    port_byte_out(ATA_PRIMARY_COMMAND, 0xEC); // IDENTIFY
    ata_io_wait();

    if (ata_get_status() == 0) return -1;

    if (ata_wait_for(ATA_STATUS_BSY, 0, 10000) < 0) return -3;
    
    uint8_t mid = port_byte_in(ATA_PRIMARY_LBA_MID);
    uint8_t high = port_byte_in(ATA_PRIMARY_LBA_HIGH);
    if (mid != 0 || high != 0) return -2;

    if (ata_wait_for(ATA_STATUS_DRQ, ATA_STATUS_DRQ, 10000) < 0) return -4;

    for (int i = 0; i < 256; i++) {
        port_word_in(ATA_PRIMARY_DATA);
    }

    return 0;
}

int ata_read_sector(int drive, uint32_t lba, uint16_t* buffer) {
    ata_wait_bsy();

    uint8_t drive_bit = (drive == 0) ? 0x00 : 0x10;
    port_byte_out(ATA_PRIMARY_DRIVE_SEL, 0xE0 | drive_bit | ((lba >> 24) & 0x0F));
    port_byte_out(ATA_PRIMARY_SEC_COUNT, 1);
    port_byte_out(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    port_byte_out(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    port_byte_out(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    port_byte_out(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);
    ata_io_wait();

    if (ata_wait_for(ATA_STATUS_BSY, 0, 10000) < 0) return -1;
    if (ata_wait_for(ATA_STATUS_DRQ, ATA_STATUS_DRQ, 10000) < 0) return -2;

    for (int i = 0; i < 256; i++) {
        buffer[i] = port_word_in(ATA_PRIMARY_DATA);
    }

    return 0;
}

int ata_write_sector(int drive, uint32_t lba, uint16_t* buffer) {
    ata_wait_bsy();

    uint8_t drive_bit = (drive == 0) ? 0x00 : 0x10;
    port_byte_out(ATA_PRIMARY_DRIVE_SEL, 0xE0 | drive_bit | ((lba >> 24) & 0x0F));
    port_byte_out(ATA_PRIMARY_SEC_COUNT, 1);
    port_byte_out(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    port_byte_out(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    port_byte_out(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    port_byte_out(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);
    ata_io_wait();

    if (ata_wait_for(ATA_STATUS_BSY, 0, 10000) < 0) return -1;
    if (ata_wait_for(ATA_STATUS_DRQ, ATA_STATUS_DRQ, 10000) < 0) return -2;

    for (int i = 0; i < 256; i++) {
        port_word_out(ATA_PRIMARY_DATA, buffer[i]);
    }

    port_byte_out(ATA_PRIMARY_COMMAND, 0xE7); // Cache flush
    ata_wait_bsy();

    return 0;
}
