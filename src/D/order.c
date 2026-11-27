#include "order_internal.h"
#include "../A/input.h"
#include "../A/datafile.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define RECEIPT "checkout.pending"

int orderReadRow(char line[], OrderRecord *o)
{
    char *f[7];
    if (!splitFields(line, f, 7) ||
        !parseInt(f[0], 1, INT_MAX, &o->orderId) ||
        !parseInt(f[1], CUST_START_ID, INT_MAX - 1, &o->accountId) ||
        !*f[2] || strlen(f[2]) >= MENU_NAME_LENGTH ||
        !parseInt(f[3], 1, MENU_MAX_SIZE, &o->sizeInches) ||
        !*f[4] || strlen(f[4]) >= MENU_NAME_LENGTH ||
        !parseInt(f[5], 1, ORDER_MAX_QUANTITY, &o->quantity) ||
        !parseMoney(f[6], 0, ORDER_MAX_TOTAL, &o->total))
        return 0;
    strcpy(o->crustName, f[2]);
    strcpy(o->toppingName, f[4]);
    return 1;
}

static void writeRow(FILE *fp, const OrderRecord *o)
{
    fprintf(fp, "%d|%d|%s|%d|%s|%d|%.2f\n", o->orderId, o->accountId,
            o->crustName, o->sizeInches, o->toppingName, o->quantity, o->total);
}

/* Read in order-number sequence. Optionally copy rows or display one account. */
static int scanOrders(FILE *copy, int account, int *lastId)
{
    FILE *fp = openDataFile(ORDER_DATA_FILE);
    OrderRecord o = {0};
    char line[256];
    int result, ok, shown = 0;
    *lastId = 0;
    if (!fp && errno != ENOENT)
        return 0;
    if (fp) {
        while ((result = readTextLine(fp, line, sizeof line)) == 1) {
            if (!orderReadRow(line, &o) || o.orderId <= *lastId)
                break;
            *lastId = o.orderId;
            if (copy)
                writeRow(copy, &o);
            if (o.accountId == account) {
                printf("#%d  %s, %d inch, %s  x%d  $%.2f\n", o.orderId,
                       o.crustName, o.sizeInches, o.toppingName, o.quantity, o.total);
                shown = 1;
            }
        }
        ok = result == -1 && !ferror(fp);
        if (fclose(fp) != 0 || !ok)
            return 0;
    }
    if (account && !shown)
        puts("You have no completed orders.");
    return 1;
}

/* A receipt records the final account values. Replaying it after a restart
   finishes the two saves without charging the customer twice. */
int orderRecover(CustomerList *customers)
{
    FILE *fp = fopen(RECEIPT, "r");
    Customer *customer;
    OrderRecord o = {0};
    char line[256], pending[DATA_PATH_LENGTH];
    char *fields[2];
    int count = 0, last, ok;
    double balance = 0.0;
    if (!fp)
        return errno == ENOENT;
    ok = readTextLine(fp, line, sizeof line) == 1 && orderReadRow(line, &o) &&
         readTextLine(fp, line, sizeof line) == 1 && splitFields(line, fields, 2) &&
         parseMoney(fields[0], 0, CUST_MAX_BALANCE, &balance) &&
         parseInt(fields[1], 1, INT_MAX, &count) &&
         readTextLine(fp, line, sizeof line) == -1 && !ferror(fp);
    if (fclose(fp) != 0 || !ok)
        return 0;
    customer = customerFindById(customers, o.accountId);
    if (!customer || !scanOrders(NULL, 0, &last))
        return 0;
    if (last != o.orderId) {
        if (last != o.orderId - 1)
            return 0;
        fp = beginDataSave(ORDER_DATA_FILE, pending);
        if (!fp)
            return 0;
        if (!scanOrders(fp, 0, &last)) {
            fclose(fp);
            remove(pending);
            return 0;
        }
        writeRow(fp, &o);
        if (!finishDataSave(fp, ORDER_DATA_FILE, pending))
            return 0;
    }
    customer->balance = balance;
    customer->orderCount = count;
    return customerSave(customers, CUST_DATA_FILE) == CUST_OK && remove(RECEIPT) == 0;
}

void orderPlace(const PizzaMenu *menu, CustomerList *customers, int accountId)
{
    Customer *customer = customerFindById(customers, accountId);
    OrderRecord o = {0};
    int crust, size, topping, last;
    char answer[8], pending[DATA_PATH_LENGTH];
    double unit, balance;
    FILE *fp;

    if (!customer || !menu_choose_pizza(menu, &crust, &size, &topping))
        return;
    if (promptInt("Quantity (1-30, 0 to cancel): ", 0, ORDER_MAX_QUANTITY,
                  &o.quantity) != 1 || !o.quantity)
        return;
    unit = menu_calculate_price(menu, crust, size, topping);
    o.total = moneyCents(unit) * o.quantity / 100.0;
    o.accountId = accountId;
    strcpy(o.crustName, menu->crustNames[crust]);
    strcpy(o.toppingName, menu->toppingNames[topping]);
    o.sizeInches = menu->sizes[size];
    printf("%s, %d inch, %s\n%d x $%.2f = $%.2f\n", o.crustName, o.sizeInches,
           o.toppingName, o.quantity, unit, o.total);
    if (o.total > customer->balance) {
        puts("Insufficient balance. Deposit money before ordering.");
        return;
    }
    if (promptLine("Confirm purchase (y/n): ", answer, sizeof answer) != 1 ||
        (strcmp(answer, "y") && strcmp(answer, "Y"))) {
        puts("Order cancelled.");
        return;
    }
    if (!scanOrders(NULL, 0, &last) || last == INT_MAX || customer->orderCount == INT_MAX) {
        puts("Cannot add an order. Check the order file.");
        return;
    }
    o.orderId = last + 1;
    balance = (moneyCents(customer->balance) - moneyCents(o.total)) / 100.0;
    fp = beginDataSave(RECEIPT, pending);
    if (!fp) {
        puts("Cannot save checkout. Your balance is unchanged.");
        return;
    }
    writeRow(fp, &o);
    fprintf(fp, "%.2f|%d\n", balance, customer->orderCount + 1);
    if (!finishDataSave(fp, RECEIPT, pending)) {
        puts("Cannot save checkout. Your balance is unchanged.");
        return;
    }
    if (!orderRecover(customers)) {
        puts("Checkout pending. Check file access and restart to finish this purchase.");
        exit(EXIT_FAILURE);
    }
    printf("Order #%d confirmed. Remaining balance: $%.2f\n", o.orderId, balance);
}

void orderShowHistory(int accountId)
{
    int last;
    printf("\nOrder history for account %d\n", accountId);
    if (!scanOrders(NULL, accountId, &last))
        puts("Cannot read the complete order history. Check orders.txt.");
}

