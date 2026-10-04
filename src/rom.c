/*
  Previous - rom.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Load ROM from a file. TMS27C512 or AM27C010 were used as ROM chip.
*/
const char Rom_fileid[] = "Previous rom.c";

#include "main.h"
#include "log.h"
#include "configuration.h"
#include "file.h"
#include "paths.h"
#include "rom.h"
#include <SDL3/SDL_filesystem.h>
#include <unistd.h>


/* Change the MAC address stored in ROM if requested */
static void rom_config(uint8_t* buf) {
    if (ConfigureParams.Rom.bUseCustomMac) {
        int i, n;
        uint32_t crc, k, r;
        
        for (i = 3; i < 6; i++) {
            buf[8+i] = ConfigureParams.Rom.nRomCustomMac[i];
        }
        
        n = 22;
        i = 0;
        
        for (k=r=~0; (++k)&7 || ((n--) && (r^=buf[i++])); r=(r&1)*0xEDB88320^(r>>1));
        crc = ~r;
        
        buf[22] = (crc >> 24) & 0xFF;
        buf[23] = (crc >> 16) & 0xFF;
        buf[24] = (crc >> 8) & 0xFF;
        buf[25] = crc & 0xFF;
    }
}

/* Bundle resources come first: SDL returns Contents/Resources on macOS and
 * the executable directory on Windows/Linux, independent of the launch CWD.
 * Then use installed data, with a working-directory fallback for source runs. */
void Rom_GetDefaultPath(char *path, int nMaxLen, const char *pszRomName)
{
	const char *pDir = Paths_GetDataDir();
	const char *base = SDL_GetBasePath();
	char *bundleRoms = base ? File_MakePath(base, "roms", NULL) : NULL;
	char *bundlePath = bundleRoms ? File_MakePath(bundleRoms, pszRomName, "BIN") : NULL;
	if (bundlePath && File_Exists(bundlePath))
	{
		File_MakePathBuf(path, nMaxLen, bundleRoms, pszRomName, "BIN");
		free(bundlePath);
		free(bundleRoms);
		return;
	}
	free(bundlePath);
	free(bundleRoms);
#ifdef ROM_INSTALL_DIR
	char *pszInstallPath = File_MakePath(ROM_INSTALL_DIR, pszRomName, "BIN");
	if (pszInstallPath)
	{
		if (File_Exists(pszInstallPath))
			pDir = ROM_INSTALL_DIR;
		free(pszInstallPath);
	}
#endif
	/* If the selected directory has no image, fall back to a "roms" folder
	 * next to the executable's current working directory (source-tree runs). */
	{
		char *pszDataPath = File_MakePath(pDir, pszRomName, "BIN");
		if (pszDataPath && !File_Exists(pszDataPath))
		{
			char cwd[FILENAME_MAX];
			char *cwdRoms = NULL;
			char *cwdPath = NULL;
			if (getcwd(cwd, sizeof(cwd)))
				cwdRoms = File_MakePath(cwd, "roms", NULL);
			if (cwdRoms)
			{
				cwdPath = File_MakePath(cwdRoms, pszRomName, "BIN");
				if (cwdPath && File_Exists(cwdPath))
				{
					File_MakePathBuf(path, nMaxLen, cwdRoms, pszRomName, "BIN");
					free(cwdPath);
					free(cwdRoms);
					free(pszDataPath);
					return;
				}
				free(cwdPath);
				free(cwdRoms);
			}
		}
		free(pszDataPath);
	}
	File_MakePathBuf(path, nMaxLen, pDir, pszRomName, "BIN");
}

/* Load a file to the ROM buffer */
int rom_load(uint8_t* buf, int len) {
    FILE* romfile;
    char* name;
    int   size;
    
    /* Loading ROM depending on emulated system */
    if (ConfigureParams.System.nMachineType == NEXT_CUBE030) {
        name = ConfigureParams.Rom.szRom030FileName;
    } else if (ConfigureParams.System.bTurbo) {
        name = ConfigureParams.Rom.szRomTurboFileName;
    } else {
        name = ConfigureParams.Rom.szRom040FileName;
    }
    Log_Printf(LOG_WARN, "Loading ROM from %s", name);
    
    romfile = File_Open(name, "rb");
    if (romfile==NULL) {
        Log_Printf(LOG_WARN, "Cannot open ROM file");
        return 1;
    }
    
    size = (int)fread(buf, 1, len, romfile);
    
    File_Close(romfile);
    
    if (size != len) {
        Log_Printf(LOG_WARN, "ROM file size is %d byte", size);
    }
    
    rom_config(buf);
    
    return 0;
}
