#include "input.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int readTextLine(FILE *stream, char text[], int size)
{
    char *newline;
    int ch;

    if (fgets(text, size, stream) == NULL)
        return -1;
    newline = strchr(text, '\n');
    if (newline == NULL) {
        ch = fgetc(stream);
        if (ch != '\n' && ch != EOF) {
            while ((ch = fgetc(stream)) != '\n' && ch != EOF)
                ;
            return 0;
        }
    }
    text[strcspn(text, "\r\n")] = '\0';
    return ferror(stream) ? -1 : 1;
}

int promptLine(const char *prompt, char text[], int size)
{
    int result;

    printf("%s", prompt);
    result = readTextLine(stdin, text, size);
    if (result == 0)
        puts("Input is too long.");
    return result;
}

static int atEnd(const char *text)
{
    while (isspace((unsigned char)*text))
        text++;
    return *text == '\0';
}

int parseInt(const char *text, int min, int max, int *value)
{
    char *end;
    long number;

    errno = 0;
    number = strtol(text, &end, 10);
    if (end == text || errno == ERANGE || !atEnd(end) ||
        number < min || number > max)
        return 0;
    *value = (int)number;
    return 1;
}

/* Call after checking the amount is within the supported range. */
long moneyCents(double amount)
{
    return (long)(amount * 100.0 + 0.5);
}

int validMoney(double amount, double min, double max)
{
    double rounded;

    /* This range check also rejects NaN and infinity. */
    if (!(amount >= min && amount <= max))
        return 0;
    rounded = moneyCents(amount) / 100.0;
    return amount >= rounded - 0.00000001 &&
           amount <= rounded + 0.00000001;
}

int parseMoney(const char *text, double min, double max, double *value)
{
    char *end;
    double amount;

    errno = 0;
    amount = strtod(text, &end);
    if (end == text || errno == ERANGE || !atEnd(end) ||
        !validMoney(amount, min, max))
        return 0;
    *value = moneyCents(amount) / 100.0;
    return 1;
}

int promptInt(const char *prompt, int min, int max, int *value)
{
    char line[64];
    int result;

    result = promptLine(prompt, line, sizeof line);
    if (result != 1)
        return result;
    if (parseInt(line, min, max, value))
        return 1;
    printf("Enter a whole number from %d to %d.\n", min, max);
    return 0;
}

int promptMoney(const char *prompt, double min, double max, double *value)
{
    char line[64];
    int result;

    result = promptLine(prompt, line, sizeof line);
    if (result != 1)
        return result;
    if (parseMoney(line, min, max, value))
        return 1;
    printf("Enter an amount from %.2f to %.2f, to the nearest cent.\n", min, max);
    return 0;
}

int splitFields(char line[], char *fields[], int count)
{
    char *separator;
    int i;

    fields[0] = line;
    for (i = 1; i < count; i++) {
        separator = strchr(fields[i - 1], '|');
        if (separator == NULL)
            return 0;
        *separator = '\0';
        fields[i] = separator + 1;
    }
    return strchr(fields[count - 1], '|') == NULL;
}
