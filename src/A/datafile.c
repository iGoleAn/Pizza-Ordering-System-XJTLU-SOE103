#include "datafile.h"
#include <errno.h>
#include <string.h>

static int makePath(char path[], const char *filename, const char *suffix)
{
    if (strlen(filename) + strlen(suffix) >= DATA_PATH_LENGTH) {
        errno = EINVAL;
        return 0;
    }
    strcpy(path, filename);
    strcat(path, suffix);
    return 1;
}

FILE *openDataFile(const char *filename)
{
    char backup[DATA_PATH_LENGTH];
    FILE *file;

    file = fopen(filename, "r");
    if (file == NULL && errno == ENOENT &&
        makePath(backup, filename, ".bak"))
        file = fopen(backup, "r");
    return file;
}

FILE *beginDataSave(const char *filename, char pending[])
{
    if (!makePath(pending, filename, ".new"))
        return NULL;
    return fopen(pending, "w");
}

int finishDataSave(FILE *file, const char *filename, const char *pending)
{
    int error;
#ifdef _WIN32
    char backup[DATA_PATH_LENGTH];
    FILE *original;
#endif

    error = ferror(file);
    if (fclose(file) != 0 || error) {
        remove(pending);
        return 0;
    }

    if (rename(pending, filename) == 0) {
#ifdef _WIN32
        if (makePath(backup, filename, ".bak"))
            remove(backup);
#endif
        return 1;
    }

#ifdef _WIN32
    /* On Windows rename will not replace an existing file. Keep a backup
       until the new file has taken its place. */
    original = fopen(filename, "r");
    if (original == NULL) {
        remove(pending);
        return 0;
    }
    if (fclose(original) != 0 || !makePath(backup, filename, ".bak") ||
        (remove(backup) != 0 && errno != ENOENT) ||
        rename(filename, backup) != 0) {
        remove(pending);
        return 0;
    }
    if (rename(pending, filename) != 0) {
        rename(backup, filename);
        remove(pending);
        return 0;
    }

    remove(backup);
    return 1;
#else
    remove(pending);
    return 0;
#endif
}
