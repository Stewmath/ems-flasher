#include "multirom.h"
#include "ems.h"
#include "main.h"

#include <err.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define BANK_SIZE 0x400000
#define BANK_SIZE_MASK 0x3fffff

typedef struct Rom {
    int offset;
    char title[0xf];
    int size;
} Rom;

Rom* roms[0x80];
int numRoms = 0;


int roundup(int offset, int size) {
    if (offset%size == 0)
        return offset;
    return offset - (offset%size) + size;
}

uint8_t getChecksum(uint8_t* header) {
    unsigned char checksum = 0;
    for (int i = 0x34; i <= 0x4c; i++) {
        checksum = checksum - header[i] - 1;
    }
    //printf("Checksum %.2x\n", checksum);
    return checksum;
}

void readRoms(int bank) {
    unsigned char buf[512];

    numRoms = 0;

    for (int i=0; i<BANK_SIZE/0x8000; i++) {
        ems_read(FROM_ROM, bank*BANK_SIZE+i*0x8000+0x100, buf, 0x100);

        bool good = true;
        for (int j=0; j<sizeof(nintylogo); j++) {
            if (nintylogo[j] != buf[0x004+j]) {
                good = false;
                break;
            }
        }

        if (good) {
            Rom* rom = malloc(sizeof(Rom));
            memcpy(rom->title, buf+0x034, 0xe);
            rom->title[0xe] = '\0';
            rom->size = getRomSize(buf[0x048]);
            rom->offset = bank*BANK_SIZE+i*0x8000;

            roms[numRoms++] = rom;
        }
    }

}

void listRoms() {
    for (int i=0; i<numRoms; i++) {
        printf("%d: %s\n", i, roms[i]->title);
    }
}

int addRom(const char* filename) {
    int blocksize = opts.blocksize;
    unsigned char buf[512];
    int offset=0;

    FILE *write_file = fopen(filename, "rb");

    if (write_file == NULL) {
        err(1, "Can't open ROM file %s", opts.file);
    }

    fseek(write_file, 0, SEEK_END);
    int size = ftell(write_file);
    fseek(write_file, 0, SEEK_SET);

    if (numRoms == 0)
        offset = opts.bank*BANK_SIZE;
    else {
        for (int i=0; i<numRoms; i++) {
            if (i == numRoms-1) {
                int romEnd = roms[i]->offset + roms[i]->size;
                int nextTry = roundup(romEnd, size);
                if (BANK_SIZE - (nextTry&BANK_SIZE_MASK) >= size) {
                    offset = nextTry;
                    break;
                }
                else {
                    err(1, "Not enough space\n");
                    return 1;
                }
            }
            else {
                int nextTry = roundup(roms[i]->offset+roms[i]->size, size);
                if (roms[i+1]->offset - nextTry >= size) {
                    offset = nextTry;
                    break;
                }
            }
        }
    }

    int base = offset/BANK_SIZE * BANK_SIZE;
    offset &= BANK_SIZE_MASK;

    if (base/BANK_SIZE != opts.bank)
        err(1, "Not enough space\n");

    while (offset + blocksize <= BANK_SIZE &&
            fread(buf, blocksize, 1, write_file) == 1) {
        int r = ems_write(TO_ROM, offset+base, buf, blocksize);
        if (r < 0) {
            warnx("can't write %d bytes at offset %u", blocksize, offset);
            return 1;
        }

        offset += blocksize;
    }

    fclose(write_file);

    if (opts.verbose)
        printf("Wrote ROM to 0x%x\n", offset&BANK_SIZE_MASK);


    return 0;
}

int deleteRom(int id) {
    unsigned char buf[512];

    if (id >= numRoms || id < 0)
        return 1;
    memset(buf, 0, 512);

    int blocksize = opts.blocksize;

    for (int i=0; i<512/blocksize; i++)
        ems_write(TO_ROM, roms[id]->offset+i*blocksize, buf+i*blocksize, blocksize);
    return 0;
}
