/* Compile-time sanity check: the core headers as compiled by the emulator
 * must be consistent with each other and with the generated config.h. */
#include "main.h"
#include "str.h"
#include "file.h"
#include "paths.h"
#include "configuration.h"
#include "rom.h"
#include "snd.h"
#include "ethernet.h"
#include "scsi.h"
#include "floppy.h"
#include "ioMem.h"
#include "m68000.h"
#include "sysReg.h"
#include "log.h"
#include "sysconfig.h"
#include "newcpu.h"
#include "slirp.h"

#ifndef PROG_NAME
#error PROG_NAME must be defined by main.h
#endif

#if !defined(ENABLE_DSP_EMU) || !defined(ENABLE_TRACING)
#error feature defines must be provided by config.h
#endif

#ifndef BIN2DATADIR
#error BIN2DATADIR must be provided by config.h
#endif

int main(void)
{
    return 0;
}