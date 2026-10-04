#ifndef UI_CONFIG89_H
#define UI_CONFIG89_H
#include <stdbool.h>
#include <stdio.h>

/* Pending native file-dialog request. */
typedef enum {
    OV_DIALOG_NONE = 0,
    OV_DIALOG_SCSI0,
    OV_DIALOG_SCSI1,
    OV_DIALOG_SCSI2,
    OV_DIALOG_SCSI3,
    OV_DIALOG_SCSI4,
    OV_DIALOG_SCSI5,
    OV_DIALOG_SCSI6,
    OV_DIALOG_FLOPPY0,
    OV_DIALOG_FLOPPY1,
    OV_DIALOG_MO0,
    OV_DIALOG_MO1,
    OV_DIALOG_ROM030,
    OV_DIALOG_ROM040,
    OV_DIALOG_ROMTURBO,
    OV_DIALOG_PRINTER_DIR,
    OV_DIALOG_NFS0,
    OV_DIALOG_NFS1,
    OV_DIALOG_NFS2,
    OV_DIALOG_NFS3,
    OV_DIALOG_NDROM0,
    OV_DIALOG_NDROM1,
    OV_DIALOG_NDROM2,
    OV_DIALOG_COUNT
} OvDialogKind;

/* 1989 UI settings persisted in the [UI89] config section. */
typedef struct {
    bool bTinker;       /* gate the Advanced tab */
    bool bSmoothing;    /* linear framebuffer filtering */
    bool bCrtEnabled;   /* Real CRT (scanline) effect */
    int  nCrtScanlines; /* scanline visibility, 0..95 (%) */
    bool bDebug;        /* show emulator debug/log output on the terminal */
    bool bRtcLocalTime; /* RTC reports host local time (true) or UTC (false) */
    bool bGifFfmpeg;    /* optimize recorded GIFs through ffmpeg */
    int  nWindowScale;  /* window scale percent (0 = fit to display) */
    int  nGifWidth;     /* recorded GIF width (320/480/640) */
    int  nGifFps;       /* recorded GIF frame rate (10/20/25) */
    int  nNotifyMode;   /* NotifyMode */
    /* Last directory browsed for each media/directory entry (indexed by OvDialogKind). */
    char szLastDir[OV_DIALOG_COUNT][FILENAME_MAX];
} UI89Config;

extern UI89Config UI89Config_;
void UI89_Load(void);
void UI89_Save(void);
void UI89_Apply(void);


#endif
