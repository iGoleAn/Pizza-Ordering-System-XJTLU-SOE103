#ifndef PIZZA_DATAFILE_H
#define PIZZA_DATAFILE_H

#include <stdio.h>

#define DATA_PATH_LENGTH 512

FILE *openDataFile(const char *filename);
FILE *beginDataSave(const char *filename, char pending[]);
int finishDataSave(FILE *file, const char *filename, const char *pending);

#endif
