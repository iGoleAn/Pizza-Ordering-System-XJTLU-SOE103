#ifndef ORDER_INTERNAL_H
#define ORDER_INTERNAL_H
#include "../A/order_port.h"

typedef struct {
    int orderId, accountId;
    char crustName[MENU_NAME_LENGTH], toppingName[MENU_NAME_LENGTH];
    int sizeInches, quantity;
    double total;
} OrderRecord;

int orderReadRow(char line[], OrderRecord *order);
#endif
