/* Exercise bundle discovery with the real ROM/file code and synthetic files. */
#include "main.h"
#include "file.h"
#include "rom.h"
#include <SDL3/SDL_filesystem.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *bundle;
static const char *data;
const char *SDL_GetBasePath(void) { return bundle; }
const char *Paths_GetDataDir(void) { return data; }

static void touch(const char *path)
{
    FILE *file = fopen(path, "wb");
    assert(file);
    assert(fclose(file) == 0);
}

int main(void)
{
    char temporary[] = "/tmp/1989-rom-paths-XXXXXX";
    char oldcwd[FILENAME_MAX], result[FILENAME_MAX], resolved[FILENAME_MAX];
    assert(getcwd(oldcwd, sizeof(oldcwd)));
    assert(mkdtemp(temporary));
    assert(realpath(temporary, resolved)); /* /tmp is a symlink on macOS. */
    char *resources = File_MakePath(resolved, "bundle resources", NULL);
    char *roms = File_MakePath(resources, "roms", NULL);
    char *installed = File_MakePath(resolved, "installed", NULL);
    char *local = File_MakePath(resolved, "roms", NULL);
    assert(mkdir(resources, 0700) == 0);
    assert(mkdir(roms, 0700) == 0);
    assert(mkdir(installed, 0700) == 0);
    assert(mkdir(local, 0700) == 0);
    char *bundleRom = File_MakePath(roms, "test", "BIN");
    char *installedRom = File_MakePath(installed, "test", "BIN");
    char *localRom = File_MakePath(local, "test", "BIN");
    touch(bundleRom); touch(installedRom); touch(localRom);
    bundle = resources;
    data = installed;
    assert(chdir(temporary) == 0);

    Rom_GetDefaultPath(result, sizeof(result), "test");
    assert(strcmp(result, bundleRom) == 0);
    assert(unlink(bundleRom) == 0);
    Rom_GetDefaultPath(result, sizeof(result), "test");
    assert(strcmp(result, installedRom) == 0);
    assert(unlink(installedRom) == 0);
    bundle = NULL; /* SDL discovery failure preserves the source-tree fallback. */
    Rom_GetDefaultPath(result, sizeof(result), "test");
    assert(strcmp(result, localRom) == 0);

    assert(unlink(localRom) == 0);
    assert(chdir(oldcwd) == 0);
    assert(rmdir(roms) == 0); assert(rmdir(resources) == 0);
    assert(rmdir(installed) == 0); assert(rmdir(local) == 0);
    assert(rmdir(temporary) == 0);
    free(resources); free(roms); free(installed); free(local);
    free(bundleRom); free(installedRom); free(localRom);
    puts("test-rom-paths: OK");
    return 0;
}
