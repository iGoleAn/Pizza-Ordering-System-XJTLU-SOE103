#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* Read a whole line and reject trailing characters. */
static int read_line(char *buffer, size_t capacity)
{
    size_t length;
    int ch;
    if (fgets(buffer, (int)capacity, stdin) == NULL) return 0;
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    return 1;
}

static int read_int(const char *prompt, int minimum, int maximum, int *result)
{
    char buffer[128];
    char *end;
    long value;
    printf("%s", prompt);
    if (!read_line(buffer, sizeof buffer)) return 0;
    value = strtol(buffer, &end, 10);
    if (end == buffer) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0' || value < minimum || value > maximum) return 0;
    *result = (int)value;
    return 1;
}

static int read_price(const char *prompt, double *result)
{
    char buffer[128];
    char *end;
    double value;
    printf("%s", prompt);
    if (!read_line(buffer, sizeof buffer)) return 0;
    value = strtod(buffer, &end);
    if (end == buffer) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0' || !isfinite(value) || value < 0.0 || value > 10000.0) return 0;
    *result = value;
    return 1;
}

void menu_init(PizzaMenu *menu)
{
    static const char *crusts[CRUST_COUNT] = {"Thin and Crispy", "Hand Tossed"};
    static const char *toppings[TOPPING_COUNT] = {"Supreme", "Hawaiian", "Margherita", "BBQ Chicken"};
    static const int sizes[SIZE_COUNT] = {10, 12, 14, 16};
    static const double crustPrices[CRUST_COUNT] = {0.0, 2.0};
    static const double sizePrices[SIZE_COUNT] = {8.0, 10.0, 12.0, 14.0};
    static const double toppingPrices[TOPPING_COUNT] = {4.0, 3.0, 2.0, 4.5};
    int i;
    if (menu == NULL) return;
    memset(menu, 0, sizeof *menu);
    for (i = 0; i < CRUST_COUNT; ++i) {
        strcpy_s(menu->crustNames[i], MENU_NAME_LENGTH, crusts[i]);
        menu->crustPrices[i] = crustPrices[i];
    }
    for (i = 0; i < SIZE_COUNT; ++i) {
        menu->sizes[i] = sizes[i];
        menu->sizePrices[i] = sizePrices[i];
    }
    for (i = 0; i < TOPPING_COUNT; ++i) {
        strcpy_s(menu->toppingNames[i], MENU_NAME_LENGTH, toppings[i]);
        menu->toppingPrices[i] = toppingPrices[i];
    }
}

void menu_display(const PizzaMenu *menu)
{
    int i;
    if (menu == NULL) return;
    puts("\n========== PIZZA MENU ==========");
    puts("Crust options (additional charge):");
    for (i = 0; i < CRUST_COUNT; ++i)
        printf("  %d. %-20s $%.2f\n", i + 1, menu->crustNames[i], menu->crustPrices[i]);
    puts("Size options (base price):");
    for (i = 0; i < SIZE_COUNT; ++i)
        printf("  %d. %2d inch              $%.2f\n", i + 1, menu->sizes[i], menu->sizePrices[i]);
    puts("Topping options (additional charge):");
    for (i = 0; i < TOPPING_COUNT; ++i)
        printf("  %d. %-20s $%.2f\n", i + 1, menu->toppingNames[i], menu->toppingPrices[i]);
    puts("Price = crust charge + size base price + topping charge");
}

double menu_calculate_price(const PizzaMenu *menu, int crustIndex, int sizeIndex, int toppingIndex)
{
    if (menu == NULL || crustIndex < 0 || crustIndex >= CRUST_COUNT ||
        sizeIndex < 0 || sizeIndex >= SIZE_COUNT ||
        toppingIndex < 0 || toppingIndex >= TOPPING_COUNT) return -1.0;
    return menu->crustPrices[crustIndex] + menu->sizePrices[sizeIndex] + menu->toppingPrices[toppingIndex];
}

int menu_set_price(PizzaMenu *menu, int category, int index, double newPrice)
{
    if (menu == NULL || !isfinite(newPrice) || newPrice < 0.0 || newPrice > 10000.0) return 0;
    switch (category) {
    case 1:
        if (index < 0 || index >= CRUST_COUNT) return 0;
        menu->crustPrices[index] = newPrice;
        return 1;
    case 2:
        if (index < 0 || index >= SIZE_COUNT) return 0;
        menu->sizePrices[index] = newPrice;
        return 1;
    case 3:
        if (index < 0 || index >= TOPPING_COUNT) return 0;
        menu->toppingPrices[index] = newPrice;
        return 1;
    default:
        return 0;
    }
}

/* Text format avoids compiler-dependent binary struct layout. */
int menu_save(const PizzaMenu *menu, const char *filename)
{
    FILE *file;
    int i;
    if (menu == NULL || filename == NULL) return 0;
    if (fopen_s(&file, filename, "w") != 0) return 0;
    if (fprintf(file, "PIZZA_MENU_V1\n") < 0) { fclose(file); return 0; }
    for (i = 0; i < CRUST_COUNT; ++i)
        if (fprintf(file, "%.2f\n", menu->crustPrices[i]) < 0) { fclose(file); return 0; }
    for (i = 0; i < SIZE_COUNT; ++i)
        if (fprintf(file, "%.2f\n", menu->sizePrices[i]) < 0) { fclose(file); return 0; }
    for (i = 0; i < TOPPING_COUNT; ++i)
        if (fprintf(file, "%.2f\n", menu->toppingPrices[i]) < 0) { fclose(file); return 0; }
    return fclose(file) == 0;
}

int menu_load(PizzaMenu *menu, const char *filename)
{
    FILE *file;
    PizzaMenu temp;
    char header[64], extra;
    int i;
    if (menu == NULL || filename == NULL) return 0;
    if (fopen_s(&file, filename, "r") != 0) return 0;
    if (fgets(header, sizeof header, file) == NULL || strcmp(header, "PIZZA_MENU_V1\n") != 0) {
        fclose(file); return 0;
    }
    menu_init(&temp);
    for (i = 0; i < CRUST_COUNT; ++i)
        if (fscanf_s(file, "%lf", &temp.crustPrices[i]) != 1) { fclose(file); return 0; }
    for (i = 0; i < SIZE_COUNT; ++i)
        if (fscanf_s(file, "%lf", &temp.sizePrices[i]) != 1) { fclose(file); return 0; }
    for (i = 0; i < TOPPING_COUNT; ++i)
        if (fscanf_s(file, "%lf", &temp.toppingPrices[i]) != 1) { fclose(file); return 0; }
    if (fscanf_s(file, " %c", &extra, 1) != EOF) { fclose(file); return 0; }
    fclose(file);
    for (i = 0; i < CRUST_COUNT; ++i)
        if (!isfinite(temp.crustPrices[i]) || temp.crustPrices[i] < 0 || temp.crustPrices[i] > 10000) return 0;
    for (i = 0; i < SIZE_COUNT; ++i)
        if (!isfinite(temp.sizePrices[i]) || temp.sizePrices[i] < 0 || temp.sizePrices[i] > 10000) return 0;
    for (i = 0; i < TOPPING_COUNT; ++i)
        if (!isfinite(temp.toppingPrices[i]) || temp.toppingPrices[i] < 0 || temp.toppingPrices[i] > 10000) return 0;
    *menu = temp;
    return 1;
}

void menu_customer_interface(const PizzaMenu *menu)
{
    int crust, size, topping;
    double price;
    if (menu == NULL) return;
    menu_display(menu);
    if (!read_int("Choose crust (1-2): ", 1, CRUST_COUNT, &crust) ||
        !read_int("Choose size (1-4): ", 1, SIZE_COUNT, &size) ||
        !read_int("Choose topping (1-4): ", 1, TOPPING_COUNT, &topping)) {
        puts("Invalid selection. Please try again.");
        return;
    }
    price = menu_calculate_price(menu, crust - 1, size - 1, topping - 1);
    printf("\nYour pizza: %s, %d inch, %s\n", menu->crustNames[crust - 1],
        menu->sizes[size - 1], menu->toppingNames[topping - 1]);
    printf("Unit price: $%.2f\n", price);
    puts("Preview only: no order has been placed or charged.");
}

void menu_manager_interface(PizzaMenu *menu)
{
    int category, selection, maximum;
    double newPrice;
    if (menu == NULL) return;
    menu_display(menu);
    puts("\nManager: edit price category");
    puts("1. Crust  2. Size  3. Topping  0. Cancel");
    if (!read_int("Category: ", 0, 3, &category)) { puts("Invalid category."); return; }
    if (category == 0) return;
    maximum = category == 1 ? CRUST_COUNT : category == 2 ? SIZE_COUNT : TOPPING_COUNT;
    if (!read_int("Item number: ", 1, maximum, &selection) ||
        !read_price("New price (0-10000): $", &newPrice)) {
        puts("Invalid item or price.");
        return;
    }
    if (menu_set_price(menu, category, selection - 1, newPrice))
        puts("Price updated. Save the menu to keep this change after restart.");
}
