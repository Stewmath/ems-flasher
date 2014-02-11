/* options */
typedef struct _options_t {
    int verbose;
    int blocksize;
    int mode;
    char *file;
    int id;
    int bank;
    int space;
} options_t;

extern options_t opts;

extern const unsigned char nintylogo[0x30];

int getRomSize(int sizeCode);

//offsets to parts of the cart header
enum headeroffsets {
    HEADER_LOGO = 0x104,
    HEADER_TITLE = 0x134,
    HEADER_CGBFLAG = 0x143,
    HEADER_SGBFLAG = 0x146,
    HEADER_ROMSIZE = 0x148,
    HEADER_RAMSIZE = 0x149,
    HEADER_REGION = 0x14A,
    HEADER_OLDLICENSEE = 0x14B,
    HEADER_ROMVER = 0x14C,
    HEADER_CHKSUM = 0x14D,
};
