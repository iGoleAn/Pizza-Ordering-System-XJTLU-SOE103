#ifndef PIZZA_INPUT_H
#define PIZZA_INPUT_H

#include <stdio.h>

/* Line reads return 1 for a line, 0 for an overlong line, and -1 at EOF/error. */
int readTextLine(FILE *stream, char text[], int size);
int promptLine(const char *prompt, char text[], int size);
int promptInt(const char *prompt, int min, int max, int *value);
int promptMoney(const char *prompt, double min, double max, double *value);

int parseInt(const char *text, int min, int max, int *value);
int parseMoney(const char *text, double min, double max, double *value);
int validMoney(double amount, double min, double max);
long moneyCents(double amount);
int splitFields(char line[], char *fields[], int count);

#endif
