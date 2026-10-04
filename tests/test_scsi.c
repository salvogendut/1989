/* Real SCSI command/sector paths and file I/O after retiring shadow writes. */
#include "../src/scsi.c"
#include <assert.h>

int main(void) {
    FILE *disk = tmpfile(); assert(disk);
    unsigned char blank[1024] = {0}, readback[512];
    assert(fwrite(blank, 1, sizeof(blank), disk) == sizeof(blank));
    SCSIbus.target = 1;
    SCSIdisk[1].dsk = disk;
    SCSIdisk[1].size = sizeof(blank);
    SCSIdisk[1].blocksize = 512;
    SCSIdisk[1].devtype = SD_HARDDISK;
    unsigned char write_command[6] = {0x0a, 0, 0, 0, 1, 0};
    SCSI_WriteSector(write_command);
    assert(SCSIbus.phase == PHASE_DO);
    memset(scsi_buffer.data, 0x5a, 512);
    scsi_write_sector();
    assert(SCSIbus.phase == PHASE_ST);
    assert(File_Read(readback, 512, 0, disk));
    for (int i = 0; i < 512; i++) assert(readback[i] == 0x5a);
    SCSIdisk[1].lba = 0; SCSIdisk[1].blockcounter = 1;
    memset(scsi_buffer.data, 0, 512); scsi_read_sector();
    assert(!memcmp(scsi_buffer.data, readback, 512));
    SCSIdisk[1].readonly = true;
    SCSI_WriteSector(write_command);
    assert(SCSIbus.phase == PHASE_ST);
    assert(SCSIdisk[1].status == STAT_CHECK_COND && SCSIdisk[1].sense.key == SK_DATAPROTECT);
    assert(SCSIdisk[1].sense.code == SC_WRITE_PROTECT);
    assert(File_Read(readback, 512, 0, disk));
    for (int i = 0; i < 512; i++) assert(readback[i] == 0x5a);
    fclose(disk);
    puts("test-scsi: OK");
    return 0;
}
