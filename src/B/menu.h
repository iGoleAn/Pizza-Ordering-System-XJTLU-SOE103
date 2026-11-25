#ifndef PIZZA_MENU_H
#define PIZZA_MENU_H

#define CRUST_COUNT 2
#define SIZE_COUNT 4
#define TOPPING_COUNT 4
#define MENU_NAME_LENGTH 40

typedef struct {
    char crustNames[CRUST_COUNT][MENU_NAME_LENGTH];
    int sizes[SIZE_COUNT];
    char toppingNames[TOPPING_COUNT][MENU_NAME_LENGTH];
    double crustPrices[CRUST_COUNT];
    double sizePrices[SIZE_COUNT];
    double toppingPrices[TOPPING_COUNT];
} PizzaMenu;

void menu_init(PizzaMenu *menu);
void menu_display(const PizzaMenu *menu);
/* Index values are zero-based. Returns -1.0 for invalid selection. */
double menu_calculate_price(const PizzaMenu *menu, int crustIndex, int sizeIndex, int toppingIndex);
/* Returns 1 on success, 0 on invalid category/index/price. Category: 1 crust, 2 size, 3 topping. */
int menu_set_price(PizzaMenu *menu, int category, int index, double newPrice);
/* Optional persistent menu storage. Returns 1 on success, 0 on failure. */
int menu_save(const PizzaMenu *menu, const char *filename);
int menu_load(PizzaMenu *menu, const char *filename);
/* Console helpers, call only after the main program checks user permissions. */
void menu_customer_interface(const PizzaMenu *menu);
void menu_manager_interface(PizzaMenu *menu);

#endif
