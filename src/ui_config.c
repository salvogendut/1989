/* Persistent desktop preferences; independent of the options panel. */
#include "ui_config.h"
#include "cfgopts.h"
#include "configuration.h"
#include "notify.h"
#include "log.h"
#include "timing.h"

UI89Config UI89Config_;

static const struct Config_Tag configs_UI89[] = {
    { "bTinker",     Bool_Tag,  &UI89Config_.bTinker },
    { "bSmoothing",  Bool_Tag,  &UI89Config_.bSmoothing },
    { "bCrtEnabled", Bool_Tag,  &UI89Config_.bCrtEnabled },
    { "nCrtScanlines", Int_Tag, &UI89Config_.nCrtScanlines },
    { "bDebug",      Bool_Tag,  &UI89Config_.bDebug },
    { "bRtcLocalTime", Bool_Tag, &UI89Config_.bRtcLocalTime },
    { "bGifFfmpeg",  Bool_Tag,  &UI89Config_.bGifFfmpeg },
    { "nWindowScale", Int_Tag,  &UI89Config_.nWindowScale },
    { "nGifWidth",   Int_Tag,   &UI89Config_.nGifWidth },
    { "nGifFps",     Int_Tag,   &UI89Config_.nGifFps },
    { "nNotifyMode", Int_Tag,   &UI89Config_.nNotifyMode },
    { "szLastDir1",  String_Tag, UI89Config_.szLastDir[1] },
    { "szLastDir2",  String_Tag, UI89Config_.szLastDir[2] },
    { "szLastDir3",  String_Tag, UI89Config_.szLastDir[3] },
    { "szLastDir4",  String_Tag, UI89Config_.szLastDir[4] },
    { "szLastDir5",  String_Tag, UI89Config_.szLastDir[5] },
    { "szLastDir6",  String_Tag, UI89Config_.szLastDir[6] },
    { "szLastDir7",  String_Tag, UI89Config_.szLastDir[7] },
    { "szLastDir8",  String_Tag, UI89Config_.szLastDir[8] },
    { "szLastDir9",  String_Tag, UI89Config_.szLastDir[9] },
    { "szLastDir10", String_Tag, UI89Config_.szLastDir[10] },
    { "szLastDir11", String_Tag, UI89Config_.szLastDir[11] },
    { "szLastDir12", String_Tag, UI89Config_.szLastDir[12] },
    { "szLastDir13", String_Tag, UI89Config_.szLastDir[13] },
    { "szLastDir14", String_Tag, UI89Config_.szLastDir[14] },
    { "szLastDir15", String_Tag, UI89Config_.szLastDir[OV_DIALOG_PRINTER_DIR] },
    { NULL, Error_Tag, NULL }
};

/* Apply preferences with runtime callbacks; rendering reads the rest. */
void UI89_Apply(void) {
    notify_set_mode((NotifyMode)UI89Config_.nNotifyMode);
    Log_SetDebugEnabled(UI89Config_.bDebug);
    Timing_SetLocalTime(UI89Config_.bRtcLocalTime);
}

void UI89_Load(void) {
    UI89Config_.bTinker    = false;
    UI89Config_.bSmoothing = true;
    UI89Config_.bCrtEnabled = false;
    UI89Config_.nCrtScanlines = 35;
    UI89Config_.bDebug     = false;
    UI89Config_.bRtcLocalTime = true;
    UI89Config_.bGifFfmpeg = false;
    UI89Config_.nWindowScale = 0;
    UI89Config_.nGifWidth  = 480;
    UI89Config_.nGifFps    = 25;
    UI89Config_.nNotifyMode = NOTIFY_MODE_SCREEN;
    if (sConfigFileName[0])
        input_config(sConfigFileName, configs_UI89, "[UI89]");
    UI89_Apply();
}

void UI89_Save(void) {
    if (sConfigFileName[0])
        update_config(sConfigFileName, configs_UI89, "[UI89]");
}
