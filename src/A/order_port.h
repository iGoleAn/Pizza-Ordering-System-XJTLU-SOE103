#ifndef PIZZA_ORDER_PORT_H
#define PIZZA_ORDER_PORT_H

#include "../B/menu.h"
#include "../C/customer.h"

#define ORDER_DATA_FILE "orders.txt"
#define ORDER_MAX_QUANTITY 30
#define ORDER_MAX_TOTAL (3 * MENU_MAX_PRICE * ORDER_MAX_QUANTITY)

/* Module D supplies these two functions when built with WITH_ORDERS. */
void orderPlace(const PizzaMenu *menu, CustomerList *customers, int accountId);
void orderShowHistory(int accountId);

/* One row per successful order, including the names and price at checkout:
   orderId|accountId|crustName|sizeInches|toppingName|quantity|total */
#endif
